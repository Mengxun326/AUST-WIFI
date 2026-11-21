local core = dofile("/austnet/core.lua")
local nixio = require "nixio"
local portal = "http://10.255.0.19"
local bodyPath = "/tmp/austnet/response-" .. nixio.getpid()

if arg[1] == "--enabled" then os.exit(core.config().enabled and 0 or 1) end

local function wan()
    local p = io.popen("ubus call network.interface.wan status 2>/dev/null")
    if not p then return end
    local info = core.decode(p:read("*a"))
    p:close()
    local addresses = info and info["ipv4-address"]
    local device = info and info.l3_device
    if info and info.up and addresses and addresses[1] and
       type(device) == "string" and device:match("^[%w_.:%-]+$") then return device end
end

local function encode(value)
    return (value:gsub("([^A-Za-z0-9_.~%-])", function(c)
        return string.format("%%%02X", c:byte())
    end))
end

local function request(device, path)
    -- 固定网关，不跟随重定向。用标准输入传入 curl 配置，避免密码出现在进程参数。
    local cfg = 'url = "' .. portal .. path .. '"\n'
    local metaPath = bodyPath .. ".meta"
    local cmd = "curl -q --config - --noproxy '*' --silent --fail --connect-timeout 4 --max-time 7" ..
        " --max-filesize 65536 --interface '" .. device .. "' --output " .. bodyPath ..
        " --write-out '%{http_code}' 2>/dev/null; printf ' %s' \"$?\""
    local p = io.popen("(" .. cmd .. ") > " .. metaPath, "w")
    if not p then return end
    p:write(cfg)
    p:close()
    -- Lua 5.1 的 pclose 不提供可靠的退出码，从文件读取 HTTP 状态和 curl 退出码。
    local httpCode, curlCode = core.read(metaPath):match("^(%d%d%d) (%d+)$")
    local body = curlCode == "0" and core.read(bodyPath) or nil
    os.remove(bodyPath)
    os.remove(metaPath)
    return body, httpCode, curlCode
end

local function status(device)
    local path = "/drcom/chkstatus?callback=dr1002&lang=zh&v=" .. os.time()
    local body = request(device, path)
    if not body then return nil, "认证网关不可达" end
    local data = core.decode(body:match("^%s*dr1002%s*%((.*)%)%s*;?%s*$") or body)
    if not data or (tostring(data.result) ~= "1" and tostring(data.result) ~= "0") then
        return nil, "无法识别网关状态"
    end
    return tostring(data.result) == "1"
end

local function login(device, c)
    local query = "callback=dr1003&DDDDD=" .. encode(c.account .. "@" .. c.exit) ..
        "&upass=" .. encode(c.password) .. "&0MKKey=123456&R1=0&R3=0&R6=0&para=00&v6ip=&v=" .. os.time()
    return request(device, "/drcom/login?" .. query)
end

local loginErrors = {
    {"账号或密码错误", {"账号或密码错误", "用户名或密码错误", "账号或密码不正确", "用户名或密码不正确"}},
    {"密码错误", {"密码错误", "密码不正确", "密码有误"}},
    {"账号不存在", {"账号不存在", "用户不存在", "无此用户"}},
    {"账号欠费", {"用户欠费", "账号欠费", "余额不足"}},
    {"账号已停用", {"账号已停用", "账号已禁用", "用户已停用"}},
    {"运营商或登录出口异常", {"运营商错误", "运营商认证失败", "运营商不匹配", "出口不匹配", "登录出口错误", "运营商账号不存在"}}
}

local function errorLabel(message)
    if type(message) ~= "string" then return end
    for _, entry in ipairs(loginErrors) do
        for _, marker in ipairs(entry[2]) do
            if message:find(marker, 1, true) then return entry[1] end
        end
    end
end

local function loginSummary(body, httpCode, curlCode)
    local summary = "登录接口响应（HTTP " .. (httpCode or "未知") ..
        "，curl 退出码 " .. (curlCode or "未知") .. "）"
    if not body then return summary .. "，请求失败" end
    local data = core.decode(body:match("^%s*dr1003%s*%((.*)%)%s*;?%s*$") or body)
    local result = data and data.result
    if data then
        local label
        if data.msga == "ldap auth error" then
            label = "账号或密码校验失败"
        elseif data.msga == "Rad:userid error1" then
            label = "账号或登录出口错误"
        else
            label = errorLabel(data.msga) or errorLabel(data.msg) or errorLabel(data.message) or errorLabel(data.error)
        end
        if label then
            local code = (result == 0 or result == "0") and "，网关 result=0" or ""
            return summary .. code .. "，网关提示：" .. label, label
        end
    end
    if type(result) == "number" and result % 1 == 0 and math.abs(result) <= 9999 then
        return summary .. "，网关 result=" .. tostring(result)
    end
    if result == "0" or result == "1" then
        return summary .. "，网关 result=" .. result
    end
    if body:lower():find("%f[%w_]login_ok%f[^%w_]") then
        return summary .. "，响应包含 login_ok"
    end
    if not data and not body:find("<", 1, true) then
        local label = errorLabel(body)
        if label then return summary .. "，网关提示：" .. label, label end
    end
    return summary .. "，响应未提供可识别的认证结果"
end

local nextLogin, nextProbe = 0, 0
local lastLoginError
local reachable, probeDevice
local firstOnline = true
local function publishOnline(device, authDetail)
    if probeDevice ~= device or os.time() >= nextProbe then
        local cmd = "curl -q --noproxy '*' --silent --fail --connect-timeout 3 --max-time 5" ..
            " --head --output /dev/null --interface '" .. device .. "' https://www.baidu.com/"
        reachable = os.execute(cmd) == 0
        probeDevice = device
        nextProbe = os.time() + 120
    end
    local network = reachable and "网络正常" or "外网检测失败"
    core.publish("已认证，" .. network, authDetail .. "。有线 WAN（" .. device .. "）" ..
        (reachable and "通过外网 HTTPS 检测" or "未通过外网 HTTPS 检测"), firstOnline)
    firstOnline = false
end

while true do
    local c = core.config()
    if not c.enabled then break end
    local device = wan()
    if c.account == "" or c.password == "" then
        core.publish("缺少学号 / 工号或密码", "认证配置不完整：缺少学号 / 工号或密码")
    elseif not device then
        nextProbe = 0
        core.publish("有线 WAN 未就绪", "有线 WAN 未就绪：无可用 IPv4 地址")
    else
        local online, reason = status(device)
        if online then
            nextLogin = 0
            lastLoginError = nil
            publishOnline(device, "会话状态查询：已认证，本轮未发起登录")
        elseif online == nil then
            nextProbe = 0
            core.publish("认证状态未知，" .. reason, "有线 WAN（" .. device .. "）查询校园网网关失败：" .. reason)
        else
            nextProbe = 0
            if os.time() >= nextLogin then
                nextLogin = os.time() + 60
                core.publish("未认证，登录请求发送中", "网关会话未认证，登录请求发送中")
                local response, httpCode, curlCode = login(device, c)
                local pending = "未认证，正在复核"
                local summary
                summary, lastLoginError = loginSummary(response, httpCode, curlCode)
                core.publish(pending, summary)
                local confirmed, failure
                for i, delay in ipairs({2, 4, 6}) do
                    nixio.nanosleep(delay)
                    confirmed, failure = status(device)
                    local result = confirmed and "已认证" or (failure or "未认证")
                    core.publish(pending, "登录状态复核 #" .. i .. "：" .. result, true)
                    if confirmed then break end
                end
                if confirmed then
                    publishOnline(device, "登录状态复核：已认证")
                elseif confirmed == false then
                    core.publish(lastLoginError and "未认证，" .. lastLoginError or "未认证，重试冷却中",
                        lastLoginError and "登录状态复核：未认证。网关提示：" .. lastLoginError or
                        response and "登录状态复核：未认证" or "登录请求失败，状态复核：未认证")
                else
                    core.publish("认证状态未知，" .. failure, "校园网登录后无法复核：" .. failure)
                end
            else
                core.publish(lastLoginError and "未认证，" .. lastLoginError or "未认证，重试冷却中",
                    "网关会话未认证，登录重试冷却中")
            end
        end
    end
    nixio.nanosleep(10)
end
