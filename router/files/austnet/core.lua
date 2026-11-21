local fs = require "nixio.fs"
local nixio = require "nixio"
local json = require "luci.json"
local M = {}
local configPath = "/austnet/config.json"
local runtime = "/tmp/austnet/"
M.exits = { aust = true, unicom = true, cmcc = true, jzg = true }

function M.read(path)
    local f = io.open(path, "rb")
    if not f then return "" end
    local value = f:read("*a") or ""
    f:close()
    return value
end

function M.decode(value)
    local ok, data = pcall(json.decode, value)
    return ok and type(data) == "table" and data or nil
end

function M.config()
    local c = M.decode(M.read(configPath)) or {}
    return {
        enabled = c.enabled == true,
        account = type(c.account) == "string" and c.account or "",
        password = type(c.password) == "string" and c.password or "",
        exit = M.exits[c.exit] and c.exit or "unicom"
    }
end

local function atomic(path, value)
    local tmp = path .. ".new"
    local f = nixio.open(tmp, "w", "600")
    if not f then return false end
    local wrote = fs.chmod(tmp, "600") and f:writeall(value)
    local closed = f:close()
    if wrote == #value and closed and fs.rename(tmp, path) then return true end
    fs.unlink(tmp)
    return false
end

function M.lock()
    local f = nixio.open("/var/lock/austnet.lock", "w", "600")
    if f and f:lock("lock") then return f end
    if f then f:close() end
end

function M.save(c)
    return atomic(configPath, json.encode(c) .. "\n")
end

function M.state()
    local status = M.read(runtime .. "status"):gsub("%s+$", "")
    return status, M.read(runtime .. "log")
end

function M.publish(status, detail, forceLog)
    local previous, log = M.state()
    if previous == status and not forceLog then return end
    local lines = { os.date("%Y-%m-%d %H:%M:%S") .. "  " .. (detail or status) }
    for line in log:gmatch("[^\n]+") do
        if #lines == 30 then break end
        lines[#lines + 1] = line
    end
    atomic(runtime .. "log", table.concat(lines, "\n") .. "\n")
    atomic(runtime .. "status", status .. "\n")
end

return M
