#!/bin/bash -ex

rm -rf *.qky *.module *.bin keys

export _JAVA_OPTIONS=

NEW_KEYPAIR=keys/p384.kp
mkdir -p keys
ltsign -help || true
ltsign createkey p384 ${NEW_KEYPAIR}

KEY_DIR=../test/keys
ROOT_KP=${KEY_DIR}/root_p384.kp
PUBLIC0_KP=${KEY_DIR}/public0_p384.kp
PUBLIC1_KP=${KEY_DIR}/public1_p384.kp

SIGNING_KP=${ROOT_KP}
for key in codesign1.module codesign2.module; do
	KEY_SIGNED=${key/%.module/_signed.module}
	cp ../work_dir/${key} ${key}
	ltsign sign ${key} ${KEY_SIGNED} ${SIGNING_KP}
	ltsign verify ${KEY_SIGNED} ${ROOT_KP}
	SIGNING_KP=${PUBLIC0_KP}
done

SIGNING_KP=${PUBLIC1_KP}

CMF=cmf_descriptor.module
cp ../work_dir/${CMF} ${CMF}
CMF_SIGNED=${CMF/%.module/_signed.module}
ltsign sign ${CMF} ${CMF_SIGNED} ${PUBLIC1_KP}
ltsign verify ${CMF_SIGNED} ${SIGNING_KP}

cp ../work_dir/root.qky root.qky
nadderdump root.qky
nadderdump codesign1.module
nadderdump codesign1_signed.module
nadderdump codesign2.module
nadderdump codesign2_signed.module

# this fails with nd4 - see CASE:434271
nadderdump ${CMF} || true
nadderdump ${CMF_SIGNED} || true

