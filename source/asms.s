	AREA	|C$$code|, CODE, READONLY

|x$codeseg| DATA

arg_1	RN R0
arg_2	RN R1

hi_1	RN	R2
hi_2	RN	R3
lo_1	RN	R4
lo_2	RN	R5

	EXPORT muls_fix

; --------------------------------------------
;
; int muls_fix(int, int);
; 
; Multiplies two fixed point 16.16 values.
; Precision will be dropped to 30.2 prior to multiplication.
; This will not account for overflow.
;
; --------------------------------------------

muls_fix   
    mov r0, r0, lsr #14
    mov r1, r1, lsr #14
    mul r0, r1, r0
    mov r0, r0, lsl #12
    mov pc, lr

	END
