SRC_DIR = src

.PHONY: clean

all:
	$(MAKE) -C $(SRC_DIR)

clean:
	$(MAKE) -C $(SRC_DIR) clean
