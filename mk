#! /bin/bash
# Casio fxcg50 build
make PLATFORM=simulator TARGET=fxcg
sh-elf-objdump -C -t output/release/simulator/fxcg/epsilon.elf | sort > dump_t
/bin/cp output/release/simulator/fxcg/epsilon.g3a /shared/tmp
