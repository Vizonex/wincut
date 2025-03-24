# wincut
An Alternative Version & Fork of [Duplicut](https://github.com/nil0x42/duplicut) for Windows.
Here I go once again to prove that the impossible can be done.

## The Goal is Simple
- Make a 1-1 version of the Duplicut for windows.
- Ensure License of the previous project remains obeyed.
- If we were able to do what we did for [winloop](https://github.com/Vizonex/Winloop),
  Then it is possible to do the same here the difficulty is slightly higher due to us only using C.

## TODOS
- [ ] Drop Termos entirely because a perfect version for windows doesn't exist
- [X] mmap (I already found an implementation of it, all I had to do was tweak it.)
- [ ] start putting all the forked files together and review for errors.
- [ ] Testing it on a large file
- [ ] Allow other directories to be utilized besides just the temporary folder directory.
- [ ] test on a large wordlist Example: `rockyou.txt`
- [ ] testing out the tagged pointers.
- [x] Rewrite the threadpool implementation for windows. (all I had to do was add a few things and redefine the signals)
- [x] PThreads (Found that a long time ago...)
