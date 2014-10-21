# News #

## v0.1 ##
Well, we waited a long time for a second release, so we get to release
something a bit more mature. This release is considered alpha, but the API is
considered at least somewhat stable, and sonames will increment henceforth.

This release boasts heavier test coverage than before, so it should work well
for most normal uses. There's still only moderate thought given to performance,
but it seems quick enough. We need users before we can have performance cases,
and we need performance cases before we can improve performance.

### v0.1.3 ###
Another bugfix release. We fix a make bug that appeared on certain platforms, a
memory gremlin, and a bug where `ason_type` wasn't reducing values before
outputting. This cleans our plate for development work.

### v0.1.2 ###
This release just massages out a few of the early gremlins. We've included the
missing LaTeX source for the spec, and fixed some of the option parsing for the
configure script. Feature wise the only new addition is allowing object keys to
be specified by parameters in ason\_read, which is something that was supposed
to be in place already but didn't actually work. We've also fixed a bug where
string values would have their underlying strings freed prematurely.

### v0.1.1 ###
Just a quick patch release. Some of the header files were missing in the last
one. Problem solved now.

## v0.0.1 ##
This is our initial release. Things are... rickety at best. Expectations aren't
high. But no matter! ASON must meet the world and learn its fate. The time to
emerge from the shadows is now! The journey into the light may be rough, but it
will make us stronger, and we will be aided by the friends we make along the
way. So fill a sack with provisions and let's get underway while we still have
daylight.
