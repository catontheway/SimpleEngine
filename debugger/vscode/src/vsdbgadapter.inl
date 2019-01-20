const char* base_code = R"_(
--此文件只能包含函数，可能有不同的luastate dofile这个文件，只执行某一个函数
local thread_name = ...

function dump_table(t)
    for k,v in pairs(t) do 
        print(k,v)
    end
end

function create_on_message_parser()
    local parsed_len = -1
    local readstate = 1
    local LINE_ENDING = nil
    return function(conn,buf,netq)
        while buf:readablesize() > 0 do 
            local preview = buf:preview(buf:readablesize())
            if readstate == 1 then
                local s,e = preview:find("\n")
                if s then
                    if not LINE_ENDING then
                        local s,e = preview:find("\r\n")
                        LINE_ENDING = s and "\r\n" or "\n"
                    end
                    local line = buf:readstring(e)
                    local match = line:gmatch("Content%-Length: (%d*)")()
                    if tonumber(match) then 
                        parsed_len = tonumber(match)
                        readstate = readstate + 1
                    else 
                        break
                    end 
                else 
                    break
                end
            elseif readstate == 2 then
                local s,e = preview:find("\n")
                if s then
                    local line = buf:readstring(e)
                    readstate = readstate+1
                else 
                    break
                end
            elseif readstate == 3 then
                if buf:readablesize() >= parsed_len then
                local js  = buf:readstring(parsed_len) 
                readstate = 1
                netq:push_back(0,js)
                else 
                    break
                end
            else
                break
            end 
        end
    end
end

if thread_name == 'vscode' then

function vscode_on_connection(conn)
     print('[vscode]' ..conn:tohostport().. ' ' ..(conn:connected() and 'true' or 'false'))
end

local vscode_on_message_parser = create_on_message_parser()
function vscode_on_message(conn,buf,netq)
    vscode_on_message_parser(conn,buf,netq)
end

elseif thread_name =='runtime' then

function runtime_on_connection(conn)
    print('[runtime]' ..conn:tohostport().. ' ' ..(conn:connected() and 'true' or 'false'))
end

local runtime_on_message_parser = create_on_message_parser()
function runtime_on_message(conn,buf,netq)
    runtime_on_message_parser(conn,buf,netq)
end

end
)_";


const char* main_code = R"_(
local cjson = require 'cjson'

local LINE_ENDING = '\r\n'
local INPUT_MODE_STDIO = 1
local INPUT_MODE_TCPIP = 2
local INPUT_MODE = nil

-- dump_table(package)

function final_send(netq, js)
    if not netq then
        print('not netq')    
        print(debug.traceback())
    end
    print('-->finalsend')
   
    local buf = {}
    table.insert(buf,"Content-Length: "..js:len())
    table.insert(buf,"")
    table.insert(buf,js)
    local sent = table.concat(buf,LINE_ENDING)
    print(sent)
    netq:push_back(1,sent)
end

local message_seq = 1
function send_response(netq, req)
    local resp = {}
    resp.type = 'response'
    resp.command = req.command
    resp.request_seq = req.seq
    resp.success = true
    resp.body = req.body 
    resp.seq = message_seq
    message_seq = message_seq + 1
    
    final_send(netq,cjson.encode(resp))
end

function send_event(netq, req, ev)
    local event = {}
    event.type = 'event'
    event.event = ev
    event.seq = message_seq
    message_seq = message_seq + 1
    
    final_send(netq,cjson.encode(event))
end          
  

function dispatch_runtime_message(netq,js,vscode_netq)
    print('-->dispatch_runtime_message')
    print(js)
    final_send(vscode_netq,js)
end

function dispatch_vscode_message(netq,js,runtime_netq)
    print('-->dispatch_vscode_message')
    print(js)
    local req = cjson.decode(js)
    if req.type == 'request' then
        local cmd = req.command 
        if cmd == "initialize" then        
            Initialize(netq, req, runtime_netq)
        elseif cmd == "launch" then
            Launch(netq, req, runtime_netq)
        elseif cmd == "attach" then
            Attach(netq, req, runtime_netq)
        elseif cmd == "disconnect" then
            Disconnect(netq, req, runtime_netq)
        elseif cmd == "restart" then
            Restart(netq, req, runtime_netq)
        elseif cmd == "setBreakpoints" then
            SetBreakpoints(netq, req, runtime_netq)
        elseif cmd == "setFunctionBreakpoints" then
            SetFunctionBreakpoints(netq, req, runtime_netq)
        elseif cmd == "setExceptionBreakpoints" then
            SetExceptionBreakpoints(netq, req, runtime_netq)
        elseif cmd == "configurationDone" then
            ConfigurationDone(netq, req, runtime_netq)
        elseif cmd == "continue" then
            Continue(netq, req, runtime_netq)
        elseif cmd == "next" then
            Next(netq, req, runtime_netq)
        elseif cmd == "stepIn" then
            StepIn(netq, req, runtime_netq)
        elseif cmd == "stepOut" then
            StepOut(netq, req, runtime_netq)
        elseif cmd == "stepOver" then
            StepOver(netq, req, runtime_netq)
        elseif cmd == "stepBack" then
            StepBack(netq, req, runtime_netq)
        elseif cmd == "reverseContinue" then
            ReverseContinue(netq, req, runtime_netq)
        elseif cmd == "restartFrame" then
            RestartFrame(netq, req, runtime_netq)
        elseif cmd == "goto" then
            Goto(netq, req, runtime_netq)
        elseif cmd == "pause" then
            Pause(netq, req, runtime_netq)
        elseif cmd == "stackTrace" then
            StackTrace(netq, req, runtime_netq)
        elseif cmd == "scopes" then
            Scopes(netq, req, runtime_netq)
        elseif cmd == "variables" then
            Variables(netq, req, runtime_netq)
        elseif cmd == "setVariable" then
            SetVariable(netq, req, runtime_netq)
        elseif cmd == "source" then
            Source(netq, req, runtime_netq)
        elseif cmd == "threads" then
            Threads(netq, req, runtime_netq)
        elseif cmd == "evaluate" then
            Evaluate(netq, req, runtime_netq)
        elseif cmd == "stepInTargets" then
            StepInTargets(netq, req, runtime_netq)
        elseif cmd == "gotoTargets" then
            GotoTargets(netq, req, runtime_netq)
        elseif cmd == "completions" then
            Completions(netq, req, runtime_netq)
        elseif cmd == "exceptionInfo" then
            ExceptionInfo(netq, req, runtime_netq)
        elseif cmd == "loadedSources" then
            LoadedSources(netq, req, runtime_netq)
        end
    end
end

function Initialize(netq, req, runtime_netq)
    req.body = {
        supportsConfigurationDoneRequest = true,
        supportsEvaluateForHovers = true,
        supportsStepBack = true
    }
    send_response(netq,req)
    send_event(netq,req,'initialized')
end 


function lua_split(str, cut)
    str = str..cut
    local pattern  = '(.-)'..cut
    local res = {}
    for w in string.gmatch(str, pattern) do
        table.insert(res,w)
        --print(w)
    end
    return res
end


function Launch(netq, req, runtime_netq)
    local arguments = req.arguments
    local program = arguments.executable
    local cwd = arguments.cwd
    local arg = arguments.args[1]
    local tokens = lua_split(arg,'=')
    local port = math.tointeger(tokens[2])
    local launch_cmd = 'start '..cwd..'/'..program..' '..arg..' --debug=1 --LE='..(LINE_ENDING=='\n\n' and 'LF' or 'CRLF')..' --server-ip=127.0.0.1 --server_port=45000  --debugPort='..port
    vscode_on_launch_cmd(launch_cmd,'127.0.0.1',port)
    final_send(runtime_netq,cjson.encode(req))
end 

function Attach(netq, req, runtime_netq)
    local arguments = req.arguments
    local program = arguments.executable
    local cwd = arguments.cwd
    local arg = arguments.args[1]
    local tokens = lua_split(arg,'=')
    local port = math.tointeger(tokens[2])
    vscode_on_attach_cmd('127.0.0.1',port)
    final_send(runtime_netq,cjson.encode(req))
end 

function Disconnect(netq, req, runtime_netq)
    final_send(runtime_netq,cjson.encode(req))
end 

function Restart(netq, req, runtime_netq)
    send_response(netq,req)
end 

function SetBreakpoints(netq, req, runtime_netq)
    final_send(runtime_netq,cjson.encode(req))
end 

function SetFunctionBreakpoints(netq, req, runtime_netq)

end 

function SetExceptionBreakpoints(netq, req, runtime_netq)
    send_response(netq,req)
end 

function ConfigurationDone(netq, req, runtime_netq)
    final_send(runtime_netq,cjson.encode(req))
end 

function Continue(netq, req, runtime_netq)
    final_send(runtime_netq,cjson.encode(req))
end 

function Next(netq, req, runtime_netq)
    final_send(runtime_netq,cjson.encode(req))
end 

function StepIn(netq, req, runtime_netq)
    final_send(runtime_netq,cjson.encode(req))
end 

function StepOut(netq, req, runtime_netq)
    final_send(runtime_netq,cjson.encode(req))
end 

function StepOver(netq, req, runtime_netq)
    final_send(runtime_netq,cjson.encode(req))
end 

function StepBack(netq, req, runtime_netq)

end 

function ReverseContinue(netq, req, runtime_netq)

end 

function RestartFrame(netq, req, runtime_netq)

end 

function Goto(netq, req, runtime_netq)

end 

function Pause(netq, req, runtime_netq)
    final_send(runtime_netq,cjson.encode(req))
end 

function StackTrace(netq, req, runtime_netq)
    final_send(runtime_netq,cjson.encode(req))
end 

function Scopes(netq, req, runtime_netq)
    final_send(runtime_netq,cjson.encode(req))
end 

function Variables(netq, req, runtime_netq)
    final_send(runtime_netq,cjson.encode(req))
end 

function SetVariable(netq, req, runtime_netq)

end 

function Source(netq, req, runtime_netq)

end 

function Threads(netq, req, runtime_netq)
    final_send(runtime_netq,cjson.encode(req))
end 

function Evaluate(netq, req, runtime_netq)
    final_send(runtime_netq,cjson.encode(req))
end 

function StepInTargets(netq, req, runtime_netq)

end 

function GotoTargets(netq, req, runtime_netq)

end 

function Completions(netq, req, runtime_netq)

end 

function ExceptionInfo(netq, req, runtime_netq)

end 

function LoadedSources(netq, req, runtime_netq)
    send_response(netq,req)
end 


function tostringex(v)
    if type(v) == 'table' then
        local count = 0
        for _ in pairs(v) do count = count + 1 end
        return string.format('table(%i)', count)
    else return tostring(v) end
end

function fetch_locals(lv)
    print("level:"..tostring(lv))
    local i = 1
    local locals = {}
    while true do
        local k,v = debug.getlocal(lv,i)
        if not k then i = 1 ; break end
        if k:sub(1,1) ~= '(' then table.insert(locals, {type(k),k,tostringex(v),type(v)}) end
        i = i + 1
    end
    while true do
        local k, v = debug.getlocal(lv, -i)
        if not k then break end
        table.insert(locals, {type(k), k, tostringex(v), type(v)})
        i = i + 1
    end
    return cjson.encode(locals)
end

)_";

