# From https://github.com/janet-lang/janet/blob/master/examples/fizzbuzz.janet

(loop [i :range [0 10000] # Modified the range
       :let [fizz (zero? (% i 3))
             buzz (zero? (% i 5))]]
  (print (cond
           (and fizz buzz) "fizzbuzz"
           fizz "fizz"
           buzz "buzz"
           i)))