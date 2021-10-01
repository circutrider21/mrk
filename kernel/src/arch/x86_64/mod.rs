/// Functions to access x86 ports
#[inline]
pub unsafe fn outb(port: u16, value: u8) {
    asm!(
       "out dx, al",
       in("dx") port,
       in("al") value,
       options(preserves_flags, nomem, nostack)
    );
}
