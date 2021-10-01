#![no_std]  // Disable the rust standard library
#![no_main] // 'fn main' isn't our entry point, so disable its warning here
#![feature(
    asm,
    thread_local,
    global_asm,
    panic_info_message,
)]

use stivale_boot::v2::*;
use log::{info, error};
use git_version::git_version;

#[repr(C, align(4096))]
struct KStack<T>(T);

/// The kernel stack (16 KiB)
static STACK: KStack<[u8; 4096 * 4]> = KStack([0; 4096 * 4]);

/// Request SMP support from the bootloader
static SMP_SUPPORT: StivaleSmpHeaderTag = StivaleSmpHeaderTag::new()
    .flags(StivaleSmpHeaderTagFlags::X2APIC);

/// Request a framebuffer from the bootloader
static FRAMEBUFFER: StivaleFramebufferHeaderTag = StivaleFramebufferHeaderTag::new()
    .framebuffer_bpp(24)
    .next((&SMP_SUPPORT as *const StivaleSmpHeaderTag).cast());

/// The stivale2 header
#[link_section = ".stivale2hdr"]
#[no_mangle]
#[used]
static STIVALE_HDR: StivaleHeader = StivaleHeader::new()
    .stack(&STACK.0[4096 * 4 - 4096] as *const u8)
    .tags((&FRAMEBUFFER as *const StivaleFramebufferHeaderTag).cast());

// Import the logger and stacktrace impl
mod debug;

/// The version variables
const GIT_VERSION: &str = git_version!();
const VERSION: Option<&'static str> = option_env!("CARGO_PKG_VERSION");

fn hey() {
    info!("Hey!\n");
}

/// Kernel entry point, the bootloader jumps here.
#[no_mangle]
pub extern "C" fn mrk_entry(bootinfo: &'static StivaleStruct) -> ! {
    // Set the base for stack-traces (Note: HAS TO BE THE FIRST THING DONE)
    unsafe {
        asm!(
            "mov rbp, 0"
        );
    }

    hey();

    let bname: &str = bootinfo.bootloader_brand();
    let bver = bootinfo.bootloader_version();

    // Setup the logger
    debug::init();
    info!("Mrk kernel v{} ({}) (x86_64)\n", VERSION.unwrap_or("unknown"), GIT_VERSION);
    info!("stivale2: Using {} {}\n", bname, bver);
    panic!("End of Kernel!\n");
}
