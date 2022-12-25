import sigrokdecode as srd

'''
test
'''


class ChannelError(Exception):
    pass


def to_hex_string(bits):
    bit_string = ''.join(buffer_to_string(bits))
    return hex(int(bit_string, 2))


def buffer_to_string(bits):
    return list(map(str, bits))


class Decoder(srd.Decoder):
    api_version = 3
    id = 's282a'
    name = 'S282A LCD Projector'
    longname = 'S282A LCD Projector Interface'
    desc = 'Synchronous, serial bus.'
    license = 'gplv2+'
    inputs = ['logic']
    outputs = ['s282a']
    tags = ['Embedded']
    channels = (
        {'id': 'latch', 'name': 'Latch', 'desc': 'CS'},
        {'id': 'clock', 'name': 'Clock', 'desc': 'WR'},
        {'id': 'data', 'name': 'Data', 'desc': 'DATA'},
    )
    optional_channels = ()
    options = ()
    annotations = (
        ('header', 'Header'),
        ('address', 'Address'),
        ('data', 'Data'),
        ('frame', 'Frame'),
        ('warnings', 'Human-readable warnings'),
        ('write', 'Write active'),
        ('segment', 'Segment data')
    )
    annotation_rows = (
        ('data', 'Data', (0, 1, 6)),
        ('raw-bits', 'Bits', (2,)),
        ('write', 'Write', (5,)),
        ('transfer', 'Transfer', (3,)),
        ('other', 'Other', (4,)),
    )

    def __init__(self):
        self.transfer_start = self.transfer_stop = 0
        self.write_stop = self.write_start = 0
        self.bit_stop = self.bit_start = 0
        self.transfer = self.write = self.bit = False
        self.bit_value = -1
        self.frame_bits = []
        self.frame_bits = []
        self.frame_bits = []
        self.decode_state = 'HEADER'
        self.start_sample = 0
        self.reset()

    def reset(self):
        self.transfer_start = self.transfer_stop = 0
        self.write_stop = self.write_start = 0
        self.bit_stop = self.bit_start = 0
        self.transfer = self.write = False
        self.bit = False
        self.frame_bits = []
        self.frame_bits = []
        self.frame_bits = []
        self.decode_state = 'HEADER'
        self.start_sample = 0
        self.bit_value = -1

    def start(self):
        self.out_ann = self.register(srd.OUTPUT_ANN)

    def transfer_started(self):
        self.transfer_start = self.samplenum
        self.transfer = True

    def transfer_stopped(self):
        self.transfer_stop = self.samplenum
        self.put(self.transfer_start, self.transfer_stop, self.out_ann, [3, ['Frame', 'Frm', 'F']])
        self.transfer = False
        self.reset()

    def write_started(self):
        self.write_start = self.samplenum
        self.write = True

    def write_stopped(self):
        if self.write is True:
            self.write_stop = self.samplenum
            self.write = False
            self.put(self.write_start, self.write_stop, self.out_ann, [5, ['Write active', 'Wr', 'W']])

    def bit_started(self, data):
        self.bit_value = data
        self.bit_start = self.samplenum
        self.bit = True
        self.on_bit_started(self.bit_value)

    def bit_stopped(self):
        if self.bit is True:
            self.bit_stop = self.samplenum
            self.bit = False
            self.put(self.bit_start, self.bit_stop, self.out_ann, [2, [str(self.bit_value)]])
            self.on_bit_finished()

    def on_bit_finished(self):
        if len(self.frame_bits) == 3 and self.decode_state == 'HEADER':
            # header done
            self.decode_state = 'ADDRESS'
            hdr = '[' + to_hex_string(self.frame_bits) + ']'
            self.put(self.start_sample, self.samplenum, self.out_ann, [0, ['Frame header ' + hdr, 'Hdr ' + hdr, 'H']])
            self.frame_bits = []
            pass
        if len(self.frame_bits) == 6 and self.decode_state == 'ADDRESS':
            # address done
            self.decode_state = 'SEGMENT'
            addr = '[' + to_hex_string(self.frame_bits) + ']'
            self.put(self.start_sample, self.samplenum, self.out_ann, [1, ['Address ' + addr, 'Adr' + addr, addr]])
            self.frame_bits = []
            pass
        if len(self.frame_bits) == 4 and self.decode_state == 'SEGMENT':
            # address done
            bits_ = '[' + ' '.join(buffer_to_string(self.frame_bits)) + ']'
            self.put(self.start_sample, self.samplenum, self.out_ann, [6, ['Segment ' + bits_, 'Seg ' + bits_, 'S']])
            self.frame_bits = []
            pass

    def on_bit_started(self, bit):
        if len(self.frame_bits) == 0:
            self.start_sample = self.samplenum
        self.frame_bits.append(bit)

    def decode(self):
        # skip first sample to work around edge case
        _ = self.wait({})
        # 'cs' - 0 , 'wr' - 1  'data' - 2
        while True:
            (cs, wr, data) = self.wait([{0: 'f'}, {0: 'r'}, {1: 'r'}])

            cs_fall = self.matched[0]
            cs_rise = self.matched[1]
            wr_rise = self.matched[2]

            if self.transfer is False and cs_fall is True:
                self.transfer_started()
            if self.transfer is True:
                if cs_rise is True:
                    self.write_stopped()
                    self.bit_stopped()
                    self.transfer_stopped()
                if wr_rise is True:
                    self.write_stopped()
                    self.bit_stopped()
                    self.write_started()
                    self.bit_started(data)
