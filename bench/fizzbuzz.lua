function fizzbuzz(limit)
    output = {}
    for i = 0, limit - 1 do
        if i % 15 == 0 then
            print("FizzBuzz")
        elseif i % 3 == 0 then
            print("Fizz")
        elseif i % 5 == 0 then
            print("Buzz")
        else
            print(i)
        end
    end
end

fizzbuzz(10000)
