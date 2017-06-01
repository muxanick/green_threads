#
# Green threads context switch.
#
#   %rcx contains the current context structure.
#   %rdx contains the context to switch to.
#

.globl _gtswtch, gtswtch
_gtswtch:
gtswtch:
        mov     %rsp, 0x00(%rcx)
        mov     %r15, 0x08(%rcx)
        mov     %r14, 0x10(%rcx)
        mov     %r13, 0x18(%rcx)
        mov     %r12, 0x20(%rcx)
        mov     %rbx, 0x28(%rcx)
        mov     %rbp, 0x30(%rcx)

        mov     0x00(%rdx), %rsp
        mov     0x08(%rdx), %r15
        mov     0x10(%rdx), %r14
        mov     0x18(%rdx), %r13
        mov     0x20(%rdx), %r12
        mov     0x28(%rdx), %rbx
        mov     0x30(%rdx), %rbp

        ret
