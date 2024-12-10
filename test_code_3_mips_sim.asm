.data
number1:    .word 5
number2:    .word 10
number3:    .word 3
result:     .word 0

.text
main:
    lw  $t0, number1
    lw  $t1, number2
    lw  $t2, number3

    add $t3, $t0, $t1
    sub $t4, $t3, $t2

    sw  $t4, result

    j   end

end:
