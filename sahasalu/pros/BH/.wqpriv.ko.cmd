cmd_/u01/sahasalu/pros/BH/wqpriv.ko := ld -r -m elf_x86_64 -z max-page-size=0x200000 -T ./scripts/module-common.lds --build-id  -o /u01/sahasalu/pros/BH/wqpriv.ko /u01/sahasalu/pros/BH/wqpriv.o /u01/sahasalu/pros/BH/wqpriv.mod.o ;  true