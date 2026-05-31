class ProgressBar:
    def __init__(self, *, bar_length: int = 60):
        self.bar_length = bar_length

    def print_empty(self):
        print(f'[@] [{'-' * self.bar_length}] {0.0:6.2f}%', end='\r', flush=True)

    def update(self, ratio: float):
        done = int(self.bar_length * ratio)
        bar  = '#' * done + '-' * (self.bar_length - done)

        print(f'[@] [{bar}] {ratio * 100:6.2f}%', end='\r', flush=True)

    def clear(self):
        print(' ' * (self.bar_length + 15), end='\r', flush=True)