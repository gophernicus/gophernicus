# Development Guidelines

**WIP**

## CI

We've recently started using Travis CI to test builds. As you
may/may not have noticed, there are a *lot* of targets. We want
gophernicus to work on a wide range of systems, and so we test it
on a lot of systems. This is why we've got two stages: the quick
stage and the full stage. When making a PR, let it run on the quick
stage. If the quick stage finds errors, then there's probably a
serious error. Once it works properly, let it run on the full stage
to catch platform/distro-specific errors (rare).
