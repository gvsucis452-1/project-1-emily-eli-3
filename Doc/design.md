# Project1 - Ouroboros Messenger
Elijah Morgan & Emily Heyboer

Our project is a process communication system, based on a one-way, circular, linked-list. /
Each node in the ring except the first is a separate, identical process that recieves messages from the previous node and passes them on to the next. The first node in the ring, node 0, accepts messages from the user and passes them on to the rest of the ring.

Nodes may only send **one** per message recieved. But any number of diagnostic logs may be written by any node at any time to stdo.

On Ctrl+C a graceful shutdown will initiate. Each node will send a signal to the next node before shutting down.
```
Example Shutdown Order, k=3:
[N0] -kill-> [N1]
[N1] -kill-> [N2]
[N2] -kill-> [N3]
```

## Example Usage
![Example Usage](/img/example.png)

## Data Architecture & Specification

### Initial Configuration
![Initial Configuration Diagram](/img/ouroborosMessengerCommunicationDiagram-Init.drawio.png)
### Pass Message
![Pass Message Diagram](/img/ouroborosMessengerCommunicationDiagram-Pass%20Message.drawio.png)
### Check Message
![Check Message Diagram](/img/ouroborosMessengerCommunicationDiagram-Message.drawio.png)
