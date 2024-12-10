.data
num1:   .word 7
num2:   .word 15
result: .word 0

.text
main:
    lw  $t0, num1
    lw  $t1, num2

    slt $t2, $t0, $t1

    sw  $t2, result

    j   end 

end:
