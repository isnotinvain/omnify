class EventDispatcher:
    def __init__(self):
        # I think we'll stay ordered, in case two things match we get determinisitc behavior
        self.handlers = []

    # matcher: msg -> state,
    # callback: state -> list[msg] | None -- None for performance? maybe not needed?
    def register_handler(self, matcher, callback):
        self.handlers.append((matcher, callback))

    def handle(self, msg):
        for matcher, callback in self.handlers:
            # TODO: is this pattern acceptable ducktyping or crazytowne?
            state = matcher(msg)

            # TODO: idk, I hate 0 being false-y. 0 is a legit control value?
            # I could wrap msg.value in a wrapper but I don't love that either
            if state is not None and state is not False:
                return callback(state)
        return None
