import gc
import sys
import boot

try:
    boot.run_default()
except Exception as e:
    sys.print_exception(e)
    boot.lcd_show_except(e)
finally:
    gc.collect()
