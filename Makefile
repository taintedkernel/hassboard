SRC_DIR = src

.PHONY: clean

all:
	$(MAKE) -C $(SRC_DIR)

nokill:
	$(MAKE) -C $(SRC_DIR) nokill

clean:
	$(MAKE) -C $(SRC_DIR) clean
