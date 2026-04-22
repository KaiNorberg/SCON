def fib(n, a, b):
    if n <= 1:
        return n
    else:
        return fib(n - 1, b, a + b)

print("Fibonacci of 65 is:", fib(65, 0, 1))