local function build_jumps(code)
    local jumps = {}
    local stack = {}
    for i = 1, #code do
        local char = code:sub(i, i)
        if char == "[" then
            table.insert(stack, i)
        elseif char == "]" then
            local open_ip = table.remove(stack)
            jumps[open_ip] = i
            jumps[i] = open_ip
        end
    end
    return jumps
end

local function bf(code)
    local jumps = build_jumps(code)
    local mem = {}
    local dp = 0
    local ip = 1
    local output = {}

    while ip <= #code do
        local char = code:sub(ip, ip)

        if char == ">" then
            dp = dp + 1
        elseif char == "<" then
            dp = dp - 1
        elseif char == "+" then
            mem[dp] = (mem[dp] or 0) + 1
        elseif char == "-" then
            mem[dp] = (mem[dp] or 0) - 1
        elseif char == "." then
            table.insert(output, string.char(mem[dp] or 0))
        elseif char == "[" then
            if (mem[dp] or 0) == 0 then
                ip = jumps[ip]
            end
        elseif char == "]" then
            if (mem[dp] or 0) ~= 0 then
                ip = jumps[ip]
            end
        end

        ip = ip + 1
    end
    return table.concat(output)
end

local result = bf(
"++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++.")
io.write(result)
