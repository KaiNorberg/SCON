def fib(n):
    if n <= 1:
        return n
    else:
        return fib(n - 1) + fib(n - 2)

print("Fibonacci of 35 is:", fib(35))