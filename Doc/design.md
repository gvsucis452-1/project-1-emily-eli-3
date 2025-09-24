# Project1 - Ouroboros Messenger
Elijah Morgan & Emily Heyboer

A process communication system, based on a one-way, circular, linked-list. /
Each node of the list is a seperate but identical process.
The HEAD/TAIL node (`n_0`) is the only node that can recieve user input.

Nodes may only send **one** per message recieved. But any number of diagnostic logs may be written by any node at any time to stdo.

On Ctrl+C a graceful shutdown will initiate. A shutdown message is sent to the next node before shutting down.
```
Example Shutdown Order, k=3:
[N0] -kill-> [N1]
[N1] -kill-> [N2]
[N2] -kill-> [N3]
[N3] -kill-> [N0]
```

## Data Architecture & Specification

### Initial Configuration
![Initial Configuration Diagram](/img/ouroborosMessengerCommunicationDiagram-Init.drawio.png)
### Pass Message
![Pass Message Diagram](/img/ouroborosMessengerCommunicationDiagram-Pass%20Message.drawio.png)
### Check Message
![Check Message Diagram](/img/ouroborosMessengerCommunicationDiagram-Message.drawio.png)

