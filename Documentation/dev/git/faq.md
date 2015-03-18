VTK Git FAQ
===========

1. How do I keep my fork in sync with a VTK repo?

    Whenever you do `git gitlab-push` to push one of your topics it will also
    fetch **master** from the main repo and push that to your fork along with
    your topic.


    In general you don't need to ever reference **master** in your fork so it
    doesn't matter whether it stays updated.  If you do want to update it
    manually you can do:

        $ git checkout master
        $ git pull
        $ git push gitlab master

2. How do I cherry pick a topic from another users fork?

        $ git fetch https://gitlab.kitware.com/$username/$project.git $branch

        # To view the changes
        $ git log -p master..FETCH_HEAD

        # To cherry-pick the changes.
        $  git cherry-pick master..FETCH_HEAD
