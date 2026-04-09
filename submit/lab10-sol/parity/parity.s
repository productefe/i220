	.text
	.globl get_parity
#edi contains n	
get_parity:
	testl   %edi, %edi
	setpe   %al
	movzbl  %al, %eax
	ret

.section .note.GNU-stack,"",@progbits
	
