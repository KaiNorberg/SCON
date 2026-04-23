(do
    (assert! (= (format "Hello, {}!" "World") "Hello, World!") "format should replace {} with arguments")
    (assert! (= (format "{1} {0}" "World" "Hello") "Hello World") "format should support explicit indexing")
    (assert! (= (format "No args") "No args") "format should handle strings without placeholders")

    (assert! (= (print! "test-print ") ()) "print! should return nil")
    (assert! (= (println! "test-println") ()) "println! should return nil")

    (assert! (number? (time)) "time should return a number")
    
    (assert! (number? (clock)) "clock should return a number")

    (assert! (atom? (env "PATH")) "env should return a string atom")
    
    (assert! (atom? (read-file "../README.md")) "read-file should read a file as a string atom")
    (assert! (!= (len (read-file "../README.md")) 0) "read-file should read non-empty content")
)
