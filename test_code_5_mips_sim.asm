.data
num1:   .word 12          
num2:   .word 5           
result_or: .word 0
result_and: .word 0

.text
main:
    lw  $t0, num1
    lw  $t1, num2

    or  $t2, $t0, $t1
    sw  $t2, result_or

    and $t3, $t0, $t1
    sw  $t3, result_and

    beq $t2, $t3, equal

    j   notequal

equal:
    li  $v0, 1
    li  $a0, 1
    j   end

notequal:
    li  $v0, 1
    li  $a0, 0

end:
