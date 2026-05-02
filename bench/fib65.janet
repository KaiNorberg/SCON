(defn fib [n a b]
  (if (< n 2)
    a
    (fib (- n 1) b (+ a b))))

(print "Fibonacci of 65 is: " (fib 65 0 1))