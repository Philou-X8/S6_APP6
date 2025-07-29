#-------------------------------
# S6 - APP6
# BREL0901
# DURP2003
#-------------------------------

import asyncio
from aiocoap import Context, Message, CHANGED
from aiocoap.resource import ObservableResource
from aiocoap import resource

class BasicResource(ObservableResource):

    def __init__(self):
        super().__init__()
        self.event_stack = []

    async def render_get(self, request):
        if self.event_stack:
            payload = self.event_stack.pop().encode('utf-8')
            return Message(payload=payload)
        else:
            return Message(payload=b"No events available")
    async def render_put(self,request):
        
        print("Received payload:", request.payload.decode())
        self.event_stack.append(request.payload.decode())
        self.updated_state()
        return Message(code=CHANGED, payload=b"Received")

        

async def main():
    # Creer le ressource pour le site

    root = resource.Site()
    root.add_resource(['user_event'], BasicResource())

    # Creer le contexte du serveur CoAP lié a 127.0.0.1:5683
    context = await Context.create_server_context(root, bind=('0.0.0.0', 5683))

    print("Serveur CoAP actif en coap://PLACEHOLDER/user_event")
    await asyncio.get_running_loop().create_future()  # Garde le programme en exécution

if __name__ == "__main__":
    asyncio.run(main())
