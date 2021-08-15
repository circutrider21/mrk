# Commit messages

Commit messages in the kernel should have a clear message that is easy to understand

A good example is the following commit message I wrote when changing this file...

```
docs: Added commit message examples
```

Commit messages in general have a simple structure, and its as follows.

```
component: Message
```

A list of valid components are as follows...

- Kernel: Used with anything kernel related
	- Can be followed by cpu arch name, like kernel/aarch64 or kernel/x86_64
	- If you modified the generic parts of the kernel, then a arch-postfix shouldn't be used
- CI: Used with anything Continuous Integration or Github, but not relating to docs
- Docs: Used with anything documentation related, like if you updated this file!
- Tools: Used if you modified/created anything under the tools folder, like shell-scripts and such
- Build: Used if you modified/created anything meson related or relating to the kernel build process
- Meta: Used for anything that doesn't fit in the above catagory, like (clang-formating)[code-style/clang-format.md] the whole source-tree

The commit message should be clear and consise, It should also be applicable to the changes that you made in the source tree

