# What is this?
This is a compiler for a programming language I've called Ltran, or the Little Translator (obviously named after FORTRAN). It is in its infancy, but I've figured it's better to release it now when it's ugly and small, rather than when it's larger, so that I'll actually have gotten it out there instead of having to sit on it for longer. I hope you enjoy reading through it and get some use out of it. Right now, it is a fairly small, and compact language with a minimal design philosophy. It's my first time making a compiler or anything with LLVM, so don't bully me too much over the messy code, please.

# What is Ltran currently capable of doing?
Not very much. It can basically only add numbers, print things, use arguments, and convert between certain types.

# What do you want Ltran to be capable of doing?
Low-level systems development is the main target for Ltran. It could also be used for higher level systems development or networking.

# Will it be updated to be able to do more in the future?
Of course! Updates to Ltran with new features will be released nightly, with a few more procedures or major language features. When the language is in a more usable state, version 1.0 will be released. Until then, versions 0.1-0.9 will all have features that are very subject to change, and I do not recommend writing important programs in the language in it until then. Though, that's mainly what I'm going to be doing with it.

# Why is part of the code filled with Doxygen comments, but the rest isn't?
I got tired. I'll add them gradually.

# I found a bug. Where do I report it?
I'm glad you (totally) asked! Open an issue on this GitHub page. Even if you did something wrong intentionally in your code, it's worth having a more verbose error than "Segmentation fault (core dumped)".

# I want a feature in Ltran. Can I submit a pull request?
I'd prefer not, I'm mostly building Ltran to learn, but you can open issues for feature requests.

# Where can I read the documentation for Ltran?
DOCS.md.
