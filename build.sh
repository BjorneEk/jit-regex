#!/usr/bin/env bash

DEBUG_MAIN=$1
FILES=$(find "$2" -type f)


SUPPORTED_KERNELS=("Darwin" "Linux" "FreeBSD")
BUILDCONF_DIR="$4"

KERNEL=$(uname)
# shellcheck disable=SC2076
[[ " ${SUPPORTED_KERNELS[*]} " =~ " ${KERNEL} " ]] || error "Unknown kernel"
# shellcheck disable=SC1090
source "$BUILDCONF_DIR/$KERNEL"

error() {
	echo "Build failed: " "$1"
	exit 1
}

filter_suffix() {
	local suffix="$1"
	shift
	local items=("$@")
	local matches=()

	for item in "${items[@]}"; do
		if [[ $item == *"$suffix" ]]; then
			matches+=("$item")
		fi
	done
	echo "${matches[@]}"
}

basenames() {
	local items=("$@")
	local new=()
	for item in "${items[@]}"; do
		local base
		base=$(basename "$item")
		new+=("$base")
	done
	echo "${new[@]}"
}

change_suffix() {
	local old_suffix=$1
	local new_suffix=$2
	shift
	shift
	local items=("$@")
	local new=()
	for item in "${items[@]}"; do
		local s
		s=${item%"$old_suffix"}$new_suffix
		new+=("$s")
	done
	echo "${new[@]}"
}
add_prefix() {
	local dir=$1
	shift
	local items=("$@")
	local new=()
	for item in "${items[@]}"; do
		new+=("$dir""$item")
	done
	echo "${new[@]}"
}
# source source-fmt(.c|.S) type(release|debug)
gen_rule_header() {
	local obj
	obj=$(basename "$1")
	local file
	local type
	file=$1
	obj=${obj%"$2"}.o
	type=$3
	echo "build/$KERNEL/$type/$obj: $file \$(HEADER_FILES) build/$KERNEL/$type"
}

#file ext type
gen_rule_msg() {
	local obj
	obj=$(basename "$1")
	local cf
	local desc
	cf=$1
	obj=${obj%"$2"}.o
	[ "$2" == '.c' ] && desc='' || desc=''
	echo "	@echo \" $3/$obj <- $desc $(basename "$cf")\""
}

# source type(release|debug)
gen_rule() {
	local file
	local flags
	local ext
	file=$1
	type=$2
	ext=".${file##*.}"
	# shellcheck disable=SC2016
	fn='$(ASM)'
	# shellcheck disable=SC2016
	flags='$(ASMFLAGS)'
	if [[ $file == *'.c' ]]; then
		ext='.c'
		# shellcheck disable=SC2016
		fn='$(CC)'
		# shellcheck disable=SC2016
		flags='$(CFLAGS) $(ERR_CFLAGS)'
		[ "$type" == 'debug' ] && flags="$flags \$(DEBUG_CFLAGS) -DDEBUG_MAIN" || flags="$flags \$(OPT_CFLAGS)"
	fi
	gen_rule_header "$file" "$ext" "$type"
	echo "	@$fn -c -o \$@ \$< $flags"
	gen_rule_msg "$file" "$ext" "$type"
	echo ""
}

gen_var() {
	local last_item
	[[ -n $2 ]] && echo "$1:= \\" || echo "$1:="

	last_item=$(echo "$2" | awk '{print $NF}')
	for f in $2; do
		[ "$f" != "$last_item" ] && echo "	$f	\\" || echo "	$f"
	done
	echo ""
}

mkdir_rule() {
	echo "$1:"
	echo "	@mkdir -p \$@"
	echo "	@echo  \$@"
	echo ""
}
#$(P_BUILD_DIR)/main.o: src/main.c $(HEADER_FILES) $(P_BUILD_DIR)
#	@$(CC) -c -o $@ $< $(CFLAGS)
#	@echo "\033[1m\033[0m main.o <- \033[1m\033[0m main.c"
# shellcheck disable=SC2068
FILE_NAMES=$(basenames ${FILES[@]})
# shellcheck disable=SC2068
C_FILES=$(filter_suffix '.c' ${FILES[@]})
# shellcheck disable=SC2068
HEADER_FILES=$(filter_suffix '.h' ${FILES[@]})
UNIQUE_DIRS=$(for file in $HEADER_FILES; do dirname "$file"; done | sort -u)
# shellcheck disable=SC2068
ASM_FILES="$(filter_suffix '.s' ${FILES[@]}) $(filter_suffix '.S' ${FILES[@]})"

# shellcheck disable=SC2068
C_FILE_NAMES=$(basenames ${C_FILES[@]})

# shellcheck disable=SC2068
C_OBJ=$(change_suffix '.c' '.o' ${C_FILE_NAMES[@]})
#echo "c:	${C_FILES[*]}"
#echo "c n:	${C_FILE_NAMES[*]}"
#echo "c o:	${C_OBJ[*]}"
# shellcheck disable=SC2046

ASM_OBJ="$(basenames $(change_suffix '.s' '.o' $(filter_suffix '.s' "${FILES[@]}"))) $(basenames $(change_suffix '.S' '.o'$(filter_suffix '.S' "${FILES[@]}")))"


# shellcheck disable=SC2068
RELEASE_OBJECT_FILES=$(add_prefix "build/$KERNEL/release/" ${C_OBJ[@]} ${ASM_OBJ[@]})
# shellcheck disable=SC2068
DEBUG_OBJECT_FILES=$(add_prefix "build/$KERNEL/debug/" ${C_OBJ[@]} ${ASM_OBJ[@]})

{
echo "#Generated makefile"
echo ""
echo "CC:=$CC"
# shellcheck disable=SC2068
echo "CFLAGS:=${CFLAGS[*]} $(add_prefix "-I" ${UNIQUE_DIRS[@]})"
echo "ERR_CFLAGS:=${ERR_CFLAGS[*]}"
echo "OPT_CFLAGS:=${OPT_CFLAGS[*]}"
echo "DEBUG_CFLAGS:=${DEBUG_CFLAGS[*]}"
echo ""
echo "ASM:=$ASM"
echo "ASMFLAGS:=${ASMFLAGS[*]}"
echo ""
echo "DEBUGGER:=$DEBUGGER"
echo "DEBUG_FLAGS:=${DEBUG_FLAGS[*]}"
echo ""
echo "DEBUG_TARGET:=bin/$KERNEL/debug/target"
echo "RELEASE_TARGET:=bin/$KERNEL/release/target"
echo ""
gen_var "HEADER_FILES" "${HEADER_FILES[*]}"
gen_var "RELEASE_OBJECT_FILES" "${RELEASE_OBJECT_FILES[*]}"
gen_var "DEBUG_OBJECT_FILES" "${DEBUG_OBJECT_FILES[*]}"
mkdir_rule "bin/$KERNEL/release"
mkdir_rule "bin/$KERNEL/debug"
mkdir_rule "build/$KERNEL/release"
mkdir_rule "build/$KERNEL/debug"
for file in $C_FILES; do
	gen_rule "$file" 'debug'
	gen_rule "$file" 'release'
done
for file in $ASM_FILES; do
	gen_rule "$file" 'debug'
	gen_rule "$file" 'release'
done
echo "\$(DEBUG_TARGET): \$(DEBUG_OBJECT_FILES) bin/$KERNEL/debug"
echo "	@\$(CC) -o \$@ $DEBUG_MAIN \$(DEBUG_OBJECT_FILES) \$(CFLAGS) \$(ERR_CFLAGS) \$(DEBUG_CFLAGS) -DDEBUG_MAIN"
echo "	@echo \"Debug target:  \$@\""
echo ""
echo "\$(RELEASE_TARGET): \$(RELEASE_OBJECT_FILES) bin/$KERNEL/release"
echo "	@\$(CC) -o \$@ \$(RELEASE_OBJECT_FILES) \$(CFLAGS) \$(ERR_CFLAGS) \$(DEBUG_CFLAGS)"
echo "	@echo \"Release target:  \$@\""
echo ""
echo "run: \$(RELEASE_TARGET)"
echo "	./\$(RELEASE_TARGET)"
echo ""
echo "debug: \$(DEBUG_TARGET)"
echo "	\$(DEBUGGER) \$(DEBUG_FLAGS) ./\$(DEBUG_TARGET)"
echo ""

} > "$3"

info_vars() {
	local w=20
	printf "%-${w}s%s\n" "Kernel:"		"$KERNEL"
	printf "%-${w}s%s\n" "Arch:"		"$ARCH"
	printf "%-${w}s%s\n" "Cpu(s):"		"$CPU_COUNT"
	echo ""
	printf "%-${w}s%s\n" "Compiler:"	"$CC_INFO"
	printf "%-${w}s%s\n" "Compiler flags:"	"${CFLAGS[*]}"
	printf "%-${w}s%s\n" "Error flags:"	"${ERR_CFLAGS[*]}"
	printf "%-${w}s%s\n" "Opt flags:"	"${OPT_CFLAGS[*]}"
	printf "%-${w}s%s\n" "Debug flags:"	"${DEBUG_CFLAGS[*]}"
	echo ""
	printf "%-${w}s%s\n" "Assembler:"	"$ASM_INFO"
	printf "%-${w}s%s\n" "Assembler flags:"	"${ASMFLAGS[*]}"
	echo ""
	printf "%-${w}s%s\n" "Debugger:"	"$DEBUGGER_INFO"
	printf "%-${w}s%s\n" "Debugger flags:"	"${DEBUG_FLAGS[*]}"
	echo ""
	printf "%-${w}s%s\n" "Files:"		"${FILE_NAMES[@]}"
	printf "%-${w}s%s\n" "C files:"		"${C_FILES[@]}"
	printf "%-${w}s%s\n" "Header files:"	"${HEADER_FILES[@]}"
	printf "%-${w}s%s\n" "ASM files:"	"${ASM_FILES[@]}"
	printf "%-${w}s%s\n" "O files:"		"${RELEASE_OBJECT_FILES[*]}"
	printf "%-${w}s%s\n" "Debug O files:"	"${DEBUG_OBJECT_FILES[*]}"
}
info_vars




