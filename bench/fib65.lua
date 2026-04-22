function fib(n, a, b)
    if n <= 0 then
        return a
    else
        return fib(n - 1, b, a + b)
    end
end

print("Fibonacci of 65 is:", fib(65, 0, 1))
