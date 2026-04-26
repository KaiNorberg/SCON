(do
    (assert! (== (min 5 3 9) 3) "min should return the smallest argument")
    (assert! (== (max 5 3 9) 9) "max should return the largest argument")
    
    (assert! (== (clamp 10 1 5) 5) "clamp should restrict value to maximum")
    (assert! (== (clamp 0 1 5) 1) "clamp should restrict value to minimum")
    (assert! (== (clamp 3 1 5) 3) "clamp should leave value unchanged if within range")

    (assert! (== (abs -5) 5) "abs should return the absolute value")
    (assert! (== (abs 5) 5) "abs should return the absolute value")

    (assert! (== (floor 3.7) 3) "floor should return the largest integer less than or equal to value")
    (assert! (== (ceil 3.2) 4) "ceil should return the smallest integer greater than or equal to value")
    (assert! (== (round 3.5) 4) "round should return the nearest integer")

    (assert! (== (pow 2 3) 8) "pow should return base raised to exponent")
    (assert! (== (sqrt 16) 4) "sqrt should return the square root")

    (assert! (number? (sin 0)) "sin should return a number")
    (assert! (number? (cos 0)) "cos should return a number")
    (assert! (number? (tan 0)) "tan should return a number")
    (assert! (number? (asin 0)) "asin should return a number")
    (assert! (number? (atan2 0 1)) "atan2 should return a number")

    (assert! (number? (rand 1 10)) "rand should return a number")
    
    (assert! (== (seed! 42) ()) "seed! should return nil")
)
