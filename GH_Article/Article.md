# MMO Hacking
Hello and welcome to my first real guide or tutorial on this Forum.
Since i got inspired by this talk: https://youtu.be/QOfroRgBgo0
recently i decided to give that a try myself.
I hope you find this guide useful and also enjoy reading it.
Please feel free to give feedback on this guide and on the code as well.
So let's get started :)

## Introduction
In MMO hacking there are as always different approches.
Our goal however is not to write a bot for the game or something like that.

We are trying to find exploits in the protocol of the game.
For that we want to do Man in the middle attacks so we can fuzz by crafting our own packets.
To be able to do that we will create our tools in the first part of this guide.
After that we try to understand the protocol and identify possible vulns.

For this guide i have taken the game Tera as an example but everything in this guide
does also apply to any other MMO.

![Tera Logo](tera.jpg)

If you don't understand everything quite yet:
Don't worry this is a step by step guide.
Meaning I will take you by the hand :)

## Recon

## Tool creation
As you have seen in our Recon phase we are lucky and dont have to deal with any anti cheat technologie.
And that is common in MMOs.
However they prevent tinkering with the game by different means.
Sent and recieved packets are always encrypted and they do also encrypt data in memory pretty often to prevent the usual memory editing (with cheatengine for example).

So How do we get around that?:
We will hook the packet creation process in an early stage where we can read the raw unencrypted data of the packets.

For that we have to find the internal/ high level send function the game uses to send packets itself and hook there.

## understanding the Protocol
## Fuzzing
Now that we can intercept the games traffic we should think about what to target.
Since integer overflows appear to be acommon problem this is what we will tackle first.
After thinking about where they could appear some things come to mind:
- Deposit negative money
- Widthdraw negative money
- ...

This can cause interesting things to happen because databases usually store unsigned integeres where the games often work with signed integers. This is not the only problem but it seems to be a common one. So if they missed a sanity check and a negative number gets stored in the database it overflows/ gets interpreted as a really high value.

Another common thing appears to be race conditions.

Think about how you would have programed something and where there might be problems.
It's also important to think out of the box well to find things that went unnoticed by others.
Every game is different and works differently so the flaws are different too.

Ideas to check for:
- anywhere an id gets transmitted -> what happens if you change thange the id for another one?
- check for logout/ save packets -> If inventory doesnt get saved you can give everythig away and load the previously saved inventory from the database.
- what happens when you move items to invalid slots?

The idea with swaping the id is that games sometimes have a false trust in packets sent from the client since they encrypt everything and it also reduces the load on the server.
So this does not only go for ids but everything that might get trusted by the gameserver.

In this guide i didnt cover hooking the recv function of the game since it is pretty much the same as finding and hooking the send function. Also in most games the recieved packets ar not as interesting as the sent ones and you have to keep in mind that the recv function gets called constantly. That means doing alot in your hook will slow or even crahs the game. That beeing saied it can stll be interesting to see how the gameserver reacts when you send its own packets back. This could be something to try when the opcodes (first bytes) of the server and client packets are the same. Meaning they share the same protocol or language.

For this most important part of the process it's really necessary to be patient and get into the right mindset:
"trying to break it".
It can take weeks or even longer to find flaws and be able to exploit them.
But according to some talks i have heared there is pretty much always at least one exploit in any mmo that can be found using this or similar techniques.

Sources:
Manfred Defcon Talk: https://youtu.be/QOfroRgBgo0
Manfred Workflow: https://youtu.be/iYTCBPUn98c
Bot creation: https://www.youtube.com/watch?v=WMlkC5L4UZk&list=PLJ3SX0ZSwtLVnZFgCwgpgPTJKF9ugpDUP
Bot creation: https://youtu.be/T-rn6squ_E4
analyzing Packets: https://progamercity.net/ghack-tut/137-tutorial-packet-hacking-reversing-mmo.html
Pwnie island writeup: https://www.youtube.com/watch?v=RDZnlcnmPUA&list=PLhixgUqwRTjzzBeFSHXrw9DnQtssdAwgG
