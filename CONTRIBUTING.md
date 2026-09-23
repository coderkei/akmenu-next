# Contributing to AKMenu-Next

Please read this document before contributing to AKMenu-Next.

Bug fixes, new features, documentation improvements, compatibility fixes, and other useful contributions are welcome however, Please read and follow the guidelines below before submitting changes.

**Please ensure you have a decent understanding of Nintendo DS programming and C++ before contributing to this project.**

## Toolchain and compatibility

AKMenu-Next targets the legacy pre-Calico devkitPro Nintendo DS environment and is built against **libnds 1.8.3**.

Changes should remain compatible with the project's existing devkitARM/libnds toolchain.

Please do not migrate code to libnds 2.x/Calico or replace legacy libnds functionality with modern equivalents simply to match current devkitPro examples. This includes areas such as ARM7/ARM9 communication, FIFO handling, interrupts, timers, startup code, and build configuration.

If the project fails to compile with a current devkitPro installation, first consider whether the installed toolchain is incompatible rather than modifying AKMenu-Next to accommodate it.

## Keep contributions focused

Please keep changes reasonably focused on the issue or feature being addressed. Avoid rewriting large portions of the code unnecessarily without good reason as will make it harder for us to review your pull request and may result in it being rejected.

AKMenu-Next runs on resource-constrained Nintendo DS hardware. New code should therefore be reasonably mindful of CPU usage, memory consumption, binary size, and unnecessary runtime overhead.

Existing code may also contain platform-specific or legacy behaviour that looks unusual by modern standards but exists for compatibility reasons. Please investigate before removing or substantially rewriting code whose purpose is unclear.

## AI-assisted contributions

**AI-assisted contributions are allowed.**

Contributors may use tools such as ChatGPT, OpenAI Codex, Claude Code, GitHub Copilot, or other generative AI and coding assistants when developing changes for AKMenu-Next.

However, **the use of AI to generate or substantially modify code submitted to this project must be disclosed.**

If AI was used, please state this clearly in the pull request description. A short explanation of what the AI was used for is appreciated, particularly for substantial changes.

For example:

> This contribution was developed with assistance from OpenAI Codex. Codex was used to implement the new functionality and assist with debugging. The resulting changes were reviewed and tested before submission.

Using AI is **not grounds for rejecting a contribution by itself**. AI-generated and AI-assisted contributions are welcome provided they meet the same quality, compatibility, and review standards as other contributions.

The disclosure requirement exists for transparency and to helpus and other maintainers understand how submitted code was produced and reviewed.

**Pull requests containing AI-generated or substantially AI-modified code that do not disclose the use of AI may be rejected.**

Contributors remain fully responsible for the code they submit regardless of whether it was written manually or with AI assistance. Please review AI-generated code carefully rather than assuming that generated code is correct!

This only applies to actual changes to the code or other content in the repository. AI usage outside of that is up to you.

Please note that pull requests containing audio or visual assets generated or modified using generative AI, such as AI generated themes, icons, artwork, or BGM, will be rejected. This restriction applies specifically to AI generated or AI modified audio/visual assets.

## Dependencies

Please avoid introducing new dependencies unless they are genuinely necessary, as additional dependencies can create unnecessary bloat. If a new dependency is required, please explain why.

Any new dependency must be compatible with the project's supported toolchain and Nintendo DS hardware. Existing project functionality and dependencies should be used and kept where practical.

## Testing

Please build and test your changes before submitting them whenever possible.

Changes affecting hardware-specific behaviour should ideally be tested on real Nintendo DS/DSi hardware where practical. Because AKMenu-Next supports a variety of DS platforms and configurations (including DSi, Flashcart, 3DS and Flashcart DSi mode via the DSpico), these changes should be tested across all platforms where possible.

If you were unable to perform a particular test or test on a particular platform, please mention this in the pull request.

## Pull requests

When opening a pull request, please include:

* A clear explanation of what was changed and why.
* Any relevant issue or problem the change addresses.
* Details of how the changes were tested, including anything you were unable to test.
* Disclosure of AI assistance where required by the policy above.

Please keep pull requests simple enough to review reasonably.

Pull requests may be declined if they do not meet the requirements in this document.

## Questions

If you're unsure about an implementation or compatibility concern, feel free to ask us first.
