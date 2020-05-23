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
First of all you always want to do some recon on the game first before jumping right into hacking it.
That includes finding out which engine/language was used o create the game, which frameworks, which anticheat
gets used (if any) etc.
Also search for previous work done on the game that can save a lot of debugging and re work.
Im sure you all know how to do that.
However here hare some little tricks you might find useful to fill this section with at least some infromation :)
- You can use Process Hacker to view the command line arguments which where used to start the game.
This can be helpful when you want to start the game without the launcher.
(ProcessHacker: https://processhacker.sourceforge.io/)
- You can use Process Monitor to see all traffic a process is sending and recieving.
That can give you a first insight and also let's you know whether the packets are encrypted at all.
Most of the time the packets will be encrypted.
(Process Monitor: https://docs.microsoft.com/en-us/sysinternals/downloads/procmon)

## Tool creation
As you have seen in our Recon phase we are lucky and dont have to deal with any anti cheat technologie.
And that is common in MMOs.
However they prevent tinkering with the game by different means.
Sent and recieved packets are always encrypted and they do also encrypt data in memory pretty often to prevent the usual memory editing (with cheatengine for example).

So How do we get around that?:
We will hook the packet creation process in an early stage where we can read the raw unencrypted data of the packets.

For that we have to find the internal/ high level send function the game uses to send packets itself and hook there.

As Mamda has saied his guide this can be a tadious process so i will cover this step in great detail and try to provide you with as much detail as i can.

### Finding the Internal / High level Send function
Communication between server and client always happens either through TCP or UDP packets.
In theory there can be a list of functions used for sending and recieving these packets.
(Mamda has listed most of these in his packet guide)
But unless your game is very exotic or old your game will most likely use the ws2_32.dll for networking and make use of either the send() function or the WSASend() function for sending packets.
However if your game sends packets over udp instead of tcp it will most likely use the sendto() equivalents.
For Recieving Data either recv() or WSARecv() are used.
I wont cover hooking/finding the recv() function in this guide.

After all that saied let's get started.
So our first problem is: finding out which functionis used by our game.
This is relatively easy to do in cheat engine or any other debugger.
We just a breakpoint at the send() function and see if its get hit.

So let's go through this step by step.
1) attach cheat engine to our game:
![attach](attach.gif)
2) find the send function:
to locate the send function we have to open the Memory View in cheat engine.
In the View Menu we can enumerate all loaded dlls.
![enum](enum.gif)
From here we can search for the ws2_32.dll and its send() function.
If we double click the function cheat engine will take us there.
![send](send.gif)
3) set a breakpoint at the send function:
To find out whether our function gets called by the game at all we need to set a breakpoint at the send() function. It should get triggered every time we do something in game.
![setbreak](setbreak.gif)
If your breakpoint gets hit and the games execution stops once we do something in game -> great.
If not -> you have to repeat the last step with a different send function and see if that one gets called.
4) finding the internal send:
This is one of the harder parts in this guide but i wiill try my best to guide you through even if your game is different.
At first it's important to understand the send function our game is using.
For that alwas search at MSDN for the used send function.
In my case send():
![msdnsend](msdnsend.gif)
(https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-send)
as we can see the 2nd parameter is a char* to our encrypted buffer and the 3rd is the length of our buffer.
These are the most important for us.
![callstack](callstack.jpg)
As you can see with our breakpoint hit we can inspect the current stack.
The first address on the stack (the one pushed onto the stack) will always be the return address (where the function got called from). After that we can see the srguments the function expects pushed onto the stack.
I have marked the ones that are important for us.
Now we have multiple options to go from here to find our internal send function.
One would be to double klick the return address to get to where the send() function got called from and repeat that until we can find the send function the game uses itself (before the packets get encrypted).
The other way i will use is to add the address of the encrypted buffer to cheat engines address table
and then -> find out what writes to this address.
![whatwrites](whatwrites.gif)
This will only work if the address of the encrypted buffer doesnt change too often.
But it has the charm that we can will find the end of the encryption function most of the time straigt away.
After that we just have to trace back again by placing a breakpoint at the encrypt function to see the return address on the stack there. If we double click there to see where it got called from and repeat that process maybe some layers upwards we can find our internal send function pretty quick.
Most of the time its the best to use a mixture of both methods depending on what your game is doing to reach your goal "quickly". What i did to find the send function for this game was to first tracke back where the send function got called from and followed the buffer. As i saw the buffer gets passed to the function that calles send as an argument i added that buffer to the address list and had a look for what writes to it.
It appears that tera uses a wrapper around the send() function thats just there to call send() really.
However this procedure is different for every game and it's important that you understand basic revrsing and debugging for you find your send function successfully.
There can be a lot of places that write to the buffer and you might have to do multiple hooks.
However if there is a unified send function in the game it has to write to the buffer at least once for every action you take. So you can ignore the opcodes that only write to the buffer for certain actions most times.
It's still a good idea to note down the addresses of these opcodes as well so you can still chekc them out later.

(I greatly enjoyed the workshop at begin.re it mostly covers static analysis. But im sure if you struggle to understand what the game is doing this workshop will help you to get to the next level :))

### Hooking the send function
### Calling the send function
### writing the Packet Editor DLL


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
- sell the same item multiple times at once -> check if it exists in inventory = true
- delete negative items
- any fees could be used to overflow an intger if sanity checks happen before fees where applied

The idea with swaping the id is that games sometimes have a false trust in packets sent from the client since they encrypt everything and it also reduces the load on the server.
So this does not only go for ids but everything that might get trusted by the gameserver.

In this guide i didnt cover hooking the recv function of the game since it is pretty much the same as finding and hooking the send function. Also in most games the recieved packets ar not as interesting as the sent ones and you have to keep in mind that the recv function gets called constantly. That means doing alot in your hook will slow or even crahs the game. That beeing saied it can stll be interesting to see how the gameserver reacts when you send its own packets back. This could be something to try when the opcodes (first bytes) of the server and client packets are the same. Meaning they share the same protocol or language.

For this most important part of the process it's really necessary to be patient and get into the right mindset:
"trying to break it".
It can take weeks or even longer to find flaws and be able to exploit them.
But according to some talks i have heared there is pretty much always at least one exploit in any mmo that can be found using this or similar techniques.

Sources and more on that Topic:
Manfred Defcon Talk: https://youtu.be/QOfroRgBgo0
Manfred Workflow: https://youtu.be/iYTCBPUn98c
Manfred RSA Talk: https://www.rsaconference.com/industry-topics/presentation/anatomy-of-exploiting-mmorpgs
Bot creation: https://www.youtube.com/watch?v=WMlkC5L4UZk&list=PLJ3SX0ZSwtLVnZFgCwgpgPTJKF9ugpDUP
Bot creation: https://youtu.be/T-rn6squ_E4
analyzing Packets: https://progamercity.net/ghack-tut/137-tutorial-packet-hacking-reversing-mmo.html
Pwnie island writeup: https://www.youtube.com/watch?v=RDZnlcnmPUA&list=PLhixgUqwRTjzzBeFSHXrw9DnQtssdAwgG
More on reversing protocols: https://youtu.be/9pCb0vIp_kg
Mambdas Guide on packets: https://guidedhacking.com/threads/the-basics-of-packets-case-study-maplestory.14074/
How to locate send function: https://www.elitepvpers.com/forum/metin2-guides-templates/4151499-how-locate-packetsend-function-nearly-every-game.html

