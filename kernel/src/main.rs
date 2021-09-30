#![no_std]  // Disable the rust standard library
#![no_main] // 'fn main' isn't our entry point, so disable its warning here
#![feature(
    asm,
    thread_local,
    global_asm,
)]

use core::panic::PanicInfo;
use stivale_boot::v2::*;
use log::info;

#[repr(C, align(4096))]
struct KStack<T>(T);

/// The kernel stack (16 KiB)
static STACK: KStack<[u8; 4096 * 4]> = KStack([0; 4096 * 4]);

/// Request a framebuffer from the bootloader
static FRAMEBUFFER: StivaleFramebufferHeaderTag = StivaleFramebufferHeaderTag::new()
    .framebuffer_bpp(24);

/// The stivale2 header
#[link_section = ".stivale2hdr"]
#[no_mangle]
#[used]
static STIVALE_HDR: StivaleHeader = StivaleHeader::new()
    .stack(&STACK.0[4096 * 4 - 4096] as *const u8)
    .tags((&FRAMEBUFFER as *const StivaleFramebufferHeaderTag).cast());

// Import the logger impl
mod logger;

/// Kernel entry point, the bootloader jumps here.
#[no_mangle]
pub extern "C" fn mrk_entry(bootinfo: &'static StivaleStruct) -> ! {
    let bname: &str = bootinfo.bootloader_brand();

    // Setup the logger
    logger::init();
    info!("Hello, World!\n");
    info!("Bootloader Brand -> {}\n", bname);
    
    loop {}
}

/// This function is called on panic.
#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}
