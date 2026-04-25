import sensor, image, time
from pyb import UART

uart = UART(3, 115200)
red_threshold = [(71, 25, 98, 19, 101, -14)]
black_threshold = [(0, 42, -84, 19, -104, 32)]
roi_1 = [
    (40, 0, 240, 40),
    (40, 200, 160, 40),
    (0, 0, 40, 240),
    (280, 0, 40, 240),
    (30, 80, 300, 80),
]
threshold_pixel = 280
from protocol_contract import FRAME_MAGIC0, FRAME_MAGIC1, FRAME_TAIL, PROTOCOL_VERSION_BYTE

sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.skip_frames(20)
sensor.set_auto_whitebal(False)
sensor.set_auto_gain(False)
clock = time.clock()
uart.init(115200, bits=8, parity=None, stop=1)

zhong_x = 0
dong_y = 0
xi_y = 0
black_num = 0

def checksum_bytes(buf):
    total = 0
    for b in buf[:10]:
        total = (total + b) & 0xFF
    return total

def pack_dot_data():
    payload = bytearray([
        FRAME_MAGIC0, FRAME_MAGIC1, PROTOCOL_VERSION_BYTE,
        (zhong_x >> 8) & 0xFF, zhong_x & 0xFF,
        (dong_y >> 8) & 0xFF, dong_y & 0xFF,
        (xi_y >> 8) & 0xFF, xi_y & 0xFF,
        black_num & 0xFF,
        0x00,
        FRAME_TAIL
    ])
    payload[10] = checksum_bytes(payload)
    return payload

def find_biggest_blob(img, roi):
    blobs = img.find_blobs(red_threshold, roi=roi, merge=True)
    if not blobs:
        return None
    biggest = blobs[0]
    for blob in blobs:
        if blob.pixels() > biggest.pixels():
            biggest = blob
    return biggest

def update_black_count(img):
    global black_num
    black_num = 0
    for roi in roi_1[:4]:
        blobs = img.find_blobs(black_threshold, roi=roi, pixels_threshold=100, area_threshold=100, merge=False)
        hit = 0
        for blob in blobs:
            if blob.pixels() > threshold_pixel:
                img.draw_rectangle(blob.rect())
                hit = 1
                break
        black_num += hit

while True:
    clock.tick()
    img = sensor.snapshot()
    zhong_x = 0
    dong_y = 0
    xi_y = 0

    center_blob = find_biggest_blob(img, roi_1[4])
    if center_blob:
        img.draw_rectangle(center_blob.rect())
        zhong_x = center_blob.cx()

    east_blob = find_biggest_blob(img, roi_1[3])
    if east_blob:
        img.draw_rectangle(east_blob.rect())
        dong_y = east_blob.cy()

    west_blob = find_biggest_blob(img, roi_1[2])
    if west_blob:
        img.draw_rectangle(west_blob.rect())
        xi_y = west_blob.cy()

    update_black_count(img)
    uart.write(pack_dot_data())
