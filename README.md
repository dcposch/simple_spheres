simple_spheres
==============

C
-

**A very basic, pretty fast
multithreaded raytracer written in C.**

Just a demo, and I'm using it for performance tests.

It can render the canonical shiny balls -- eight of them,
bouncing around in an invisible box, with two point
light sources and a trace depth of two (ie you can see
reflections of reflections) -- at 45fps in my little VM
running on my laptop at VGA resolution. It cheats 
egregiously to make this possible, starting by rasterizing
the spheres very crudely into a stencil buffer, and then
only tracing rays in places where the stencil buffer is
set.

In the future, I want to experiment with a more general form
of this: I want to use OpenGL to render a scene up the the
fragment stage, and then go through the fragments, tracing 
secondary rays from only the (usually small) subset that are
highly specular.

**60 FPS** on my machine.


Haskell
-------

I've now added a very similar but even more simplified 
raytracer in Haskell, for comparison. Haskell is a beautiful
language, so it's sad that even with all the voodoo black magic
that GHC can work, it is still at least two orders of magnitude
slower than C. And that for a simple (a couple hundred lines in 
both languages) program that is mostly just math. (In other words,
the kind of thing even Java can do reasonably quickly. If I have
time, I will in fact port the C code to Java because I'm quite
curious now.)

Pros:
The entire rendering is a single pure function, which is pretty cool.
In theory this makes the individual ray casts trivially parallelizable.

Cons:
Even with the cool parts like antialiasing, collision detection, and
multiple lightsources disabled and the trace depth reduced to 1 at
320x240 (ie extremely limited settings), roughly 1 FPS.

**Not enough FPS** on my machine.



Java
----


Java is much more similar to C than Haskell is, so the Java port is almost line-by-line with a bit of boilerplate added and a bit removed.

A few lessons for me:

- Java's graphics libraries were written by pedantic neckbeards with a weird fetish for classes and inheritance. Even getting a super-basic app that just draws a pulsating square (no raytracing at all) at 60fps required a lot of java.awt hackery. Basically all the efaults are wrong.
- By default, Java's BufferedImage is insanely slow. I had to resort to some almost-but-not-quite portable hacks. This is partly because of "ColorModel", Java likes to do full color-space transformations on every pixel that you set. How many pro photo editing apps are written in Java, benefitting from the built in color-space transformation? None. How many Java graphics apps are slow and crappy because of it? Lots.
- Java doesn't support floats very well. The standard library really wants you to use doubles. Not only are functions like sqrt and sin not overloaded, but there's no sinf / sqrtf either.

**7 FPS** on my machine.
