.data

.globl main
.text
test: 
addiu $sp, $sp, -12

sw $ra, 0($sp)

sw $a0, 4($sp)
sw $a1, 8($sp)
lw a, a

lw b, b

c.le.s , a, b

li $v0, 1
syscall

lw $ra, 0($sp)
addiu $sp, $sp, 12
jr $ra
main: 
addiu $sp, $sp, -4

sw $ra, 0($sp)

li $v0, 5
syscall
mfc1 $f0, $f0

sw $f0, a

li $v0, 5
syscall
mfc1 $f0, $f0

sw $f0, b

lw a, a

lw b, b

mfc1 $a0, a
mfc1 $a1, b
jal test
mfc1 $v0, $f0

lw $ra, 0($sp)
addiu $sp, $sp, 4
jr $ra

