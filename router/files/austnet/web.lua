return function()
    local http = require "luci.http"
    local dispatcher = require "luci.dispatcher"
    local core = dofile("/austnet/core.lua")
    local function reply(payload)
        http.header("Cache-Control", "no-store")
        http.prepare_content("application/json")
        http.write_json(payload)
    end

    if not dispatcher.context.authsession or http.getenv("HTTP_X_AUSTNET_REQUEST") ~= "1" then
        http.status(403, "Forbidden")
        return reply({ ok = false })
    end
    local method = http.getenv("REQUEST_METHOD")
    if method ~= "GET" and method ~= "POST" then
        http.status(405, "Method Not Allowed")
        return reply({ ok = false })
    end

    local c = core.config()
    if method == "POST" then
        local length = tonumber(http.getenv("CONTENT_LENGTH")) or 0
        if length < 1 or length > 4096 or not (http.getenv("CONTENT_TYPE") or ""):find(
           "^application/x%-www%-form%-urlencoded") then
            return reply({ ok = false, message = "请求格式无效" })
        end
        local form = http.formvalue
        local enabled, account, exit, password = form("enabled"), form("account"), form("exit"), form("password")
        if (enabled ~= "0" and enabled ~= "1") or type(account) ~= "string" or
           #account > 64 or (account ~= "" and not account:match("^[0-9A-Za-z_.%-]+$")) or
           type(exit) ~= "string" or not core.exits[exit] or
           type(password) ~= "string" or #password > 512 or password:find("%c") then
            return reply({ ok = false, message = "请检查学号 / 工号、密码和登录出口" })
        end

        local lock = core.lock()
        if not lock then return reply({ ok = false, message = "无法锁定配置" }) end
        c = core.config()
        c.enabled, c.account, c.exit = enabled == "1", account, exit
        if password ~= "" then c.password = password end
        if c.enabled and (c.account == "" or c.password == "") then
            lock:close()
            return reply({ ok = false, message = "启用前请填写学号 / 工号和密码" })
        end
        if not core.save(c) then
            lock:close()
            return reply({ ok = false, message = "配置保存失败" })
        end
        local applied = os.execute("/etc/init.d/austnet stop >/dev/null 2>&1; " ..
            "mkdir -p /tmp/austnet && chmod 700 /tmp/austnet") == 0
        if applied then
            if c.enabled then
                core.publish("校园网自动认证服务启动中")
            else
                core.publish("校园网自动认证服务已关闭", "校园网自动认证服务已停止")
            end
            if c.enabled then
                applied = os.execute("/etc/init.d/austnet start >/dev/null 2>&1") == 0
            end
        end
        lock:close()
        if not applied then return reply({ ok = false, message = "配置已保存，服务应用失败" }) end
    end

    local status, log = core.state()
    local payload = {
        ok = true, enabled = c.enabled, account = c.account,
        exit = c.exit,
        status = not c.enabled and "校园网自动认证服务已关闭" or (status ~= "" and status or "等待校园网自动认证服务启动"),
        log = log
    }
    if method == "GET" and http.formvalue("config") == "1" then
        payload.password = c.password
    end
    return reply(payload)
end
