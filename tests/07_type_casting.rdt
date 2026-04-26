(do
    (assert! (= (int "42") 42) "int should convert string to integer")
    (assert! (= (int 3.14) 3) "int should convert float to integer")

    (assert! (= (float "3.14") 3.14) "float should convert string to float")
    (assert! (= (float 42) 42.0) "float should convert integer to float")

    (assert! (= (string 42) "42") "string should convert integer to string")
    (assert! (= (string "hello") "hello") "string should return the string itself")
    (assert! (= (string (list 1 2 3)) "(1 2 3)") "string should stringify a list")
)
