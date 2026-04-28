import sensor, image, lcd, time
import KPU as kpu
import gc, sys
from fpioa_manager import fm
from machine import UART

fm.register(15, fm.fpioa.UART1_TX, force=True)
fm.register(17, fm.fpioa.UART1_RX, force=True)

uart_A = UART(UART.UART1, 115200)
from protocol_contract import (
    FRAME_MAGIC0, FRAME_MAGIC1, FRAME_TAIL, PROTOCOL_VERSION_BYTE,
    K210_CLASS_THRESHOLDS,
)
SEQ = 0
CONF_THRESHOLDS = K210_CLASS_THRESHOLDS
LABEL_TO_BIT = {
    "1": 0, "2": 1, "3": 2, "4": 3,
    "5": 4, "6": 5, "7": 6, "8": 7,
}

def checksum_bytes(buf):
    total = 0
    for b in buf[:10]:
        total = (total + b) & 0xFF
    return total

def pack_dot_data(class_mask, best_conf, best_x, best_y, seq):
    pack_data = bytearray([
        FRAME_MAGIC0, FRAME_MAGIC1, PROTOCOL_VERSION_BYTE,
        class_mask & 0xFF,
        best_conf & 0xFF,
        (best_x >> 8) & 0xFF, best_x & 0xFF,
        (best_y >> 8) & 0xFF, best_y & 0xFF,
        seq & 0xFF,
        0x00,
        FRAME_TAIL
    ])
    pack_data[10] = checksum_bytes(pack_data)
    return pack_data

def lcd_show_except(e):
    import uio
    err_str = uio.StringIO()
    sys.print_exception(e, err_str)
    err_str = err_str.getvalue()
    img = image.Image(size=(224,224))
    img.draw_string(0, 10, err_str, scale=1, color=(0xff,0x00,0x00))
    lcd.display(img)

def main(anchors, labels=None, model_addr="/sd/test.kmodel", sensor_window=(224, 224), lcd_rotation=0):
    global SEQ
    sensor.reset()
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_framesize(sensor.QVGA)
    sensor.set_windowing(sensor_window)
    sensor.set_hmirror(1)
    sensor.set_vflip(1)
    sensor.run(1)

    lcd.init(type=1)
    lcd.rotation(lcd_rotation)
    lcd.clear(lcd.WHITE)

    if not labels:
        label_candidates = ('labels.txt', 'lables.txt')
        for label_file in label_candidates:
            try:
                with open(label_file, 'r') as f:
                    exec(f.read())
                break
            except Exception:
                pass
    if not labels:
        img = image.Image(size=(320, 240))
        img.draw_string(90, 110, "no labels.txt", color=(255, 0, 0), scale=2)
        lcd.display(img)
        return 1

    try:
        lcd.display(image.Image("startup.jpg"))
    except Exception:
        img = image.Image(size=(320, 240))
        img.draw_string(90, 110, "loading model...", color=(255, 255, 255), scale=2)
        lcd.display(img)

    task = kpu.load(model_addr)
    kpu.init_yolo2(task, 0.5, 0.3, 5, anchors)
    try:
        while True:
            img = sensor.snapshot()
            tick = time.ticks_ms()
            objects = kpu.run_yolo2(task, img)
            tick = time.ticks_ms() - tick
            class_mask = 0
            best_conf = 0
            best_x = 0
            best_y = 0
            if objects:
                for obj in objects:
                    pos = obj.rect()
                    score = int(obj.value() * 100)
                    label = labels[obj.classid()]
                    img.draw_rectangle(pos)
                    img.draw_string(pos[0], pos[1], "%s : %.2f" % (label, obj.value()), scale=2)
                    center_x = pos[0] + int(pos[2] / 2)
                    center_y = pos[1] + int(pos[3] / 2)
                    img.draw_cross(center_x, center_y)
                    if label in LABEL_TO_BIT and score >= CONF_THRESHOLDS.get(label, 70):
                        class_mask |= (1 << LABEL_TO_BIT[label])
                    if score > best_conf:
                        best_conf = score
                        best_x = center_x
                        best_y = center_y
            uart_A.write(pack_dot_data(class_mask, best_conf, best_x, best_y, SEQ))
            SEQ = (SEQ + 1) & 0xFF
            img.draw_string(0, 200, "t:%dms m:%02X c:%d" % (tick, class_mask, best_conf), scale=2)
            lcd.display(img)
    except Exception as e:
        raise e
    finally:
        kpu.deinit(task)


def run_default():
    labels = ["3", "4", "1", "2", "7", "8", "5", "6"]
    anchors = [1.7048, 1.6693, 1.8474, 2.1746, 2.0003, 1.8557, 2.1034, 2.612, 2.4321, 3.0607]
    main(anchors=anchors, labels=labels, model_addr="/sd/test.kmodel")
