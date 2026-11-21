module("luci.controller.austnet", package.seeall)

function index()
    entry({"web", "prosetting", "austnet"}, template("web/setting/austnet")).leaf = false
    entry({"web", "prosetting", "austnet", "data"}, call("data")).leaf = true
end

function data()
    return dofile("/austnet/web.lua")()
end
