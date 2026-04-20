function fib(n)
    if n <= 1 then
        return n
    else
        return fib(n - 1) + fib(n - 2)
    end
end

print("Fibonacci of 35 is:", fib(35))
