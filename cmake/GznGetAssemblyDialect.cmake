
function(gzn_get_assembly_dialect OUTVAR)
	enable_language(ASM)

	if(NOT OUTVAR)
		message(FATAL_ERROR "gzn_get_assembly_dialect requires one OUT parameter")
	endif()

	set(_base "${CMAKE_BINARY_DIR}/CMakeGznAsmDialect")
	file(MAKE_DIRECTORY "${_base}")

	function(_lol_try name ext code)
		set(_dir "${_base}/${name}")
		file(MAKE_DIRECTORY "${_dir}")
		set(_src "${_dir}/test${ext}")
		file(WRITE "${_src}" "${code}")
		try_compile(${name}_OK "${_dir}/build" "${_src}")
		set(${name}_OK "${${name}_OK}" PARENT_SCOPE)
	endfunction()


	_lol_try(ATT_GAS .s
".globl main
.type main,@function
main:
  movl $1, %eax
  ret
")
	if(ATT_GAS_OK)
		set(${OUTVAR} "ATT" PARENT_SCOPE)
		return()
	endif()


	_lol_try(INTEL_GAS .S
".intel_syntax noprefix
.globl main
.type main,@function
main:
  mov eax, 1
  ret
")
	if(INTEL_GAS_OK)
		set(${OUTVAR} "INTEL" PARENT_SCOPE)
		return()
	endif()


	# --- MASM / ML / ML64 ---
	if(MSVC OR DEFINED CMAKE_ASM_MASM_COMPILER)
		_lol_try(MASM .asm
".code
PUBLIC main
main PROC
  mov eax, 1
  ret
main ENDP
END
")
		if(MASM_OK)
			set(${OUTVAR} "MASM" PARENT_SCOPE)
			return()
		endif()
	endif()


	# --- NASM / YASM ---
	_lol_try(NASM .asm
"global main
section .text
main:
  mov eax, 1
  ret
")
	if(NASM_OK)
		set(${OUTVAR} "NASM" PARENT_SCOPE)
		return()
	endif()


	# --- AArch64 GNU ---
	_lol_try(AARCH64_GAS .s
".globl main
main:
  mov x0, #0
  ret
")
	if(AARCH64_GAS_OK)
		set(${OUTVAR} "AARCH64" PARENT_SCOPE)
		return()
	endif()


	# --- ARM GNU ---
	_lol_try(ARM_GAS .s
".globl main
main:
  mov r0, #0
  bx lr
")
	if(ARM_GAS_OK)
		set(${OUTVAR} "ARM" PARENT_SCOPE)
		return()
	endif()


	# --- RISC-V ---
	_lol_try(RISCV_GAS .s
".globl main
main:
  li a0, 0
  ret
")
	if(RISCV_GAS_OK)
		set(${OUTVAR} "RISCV" PARENT_SCOPE)
		return()
	endif()


	# --- PowerPC ---
	_lol_try(PPC_GAS .s
".globl main
main:
  li 3,0
  blr
")
	if(PPC_GAS_OK)
		set(${OUTVAR} "PPC" PARENT_SCOPE)
		return()
	endif()


	# --- WebAssembly ---
	_lol_try(WASM .s
".globaltype main, i32
main:
  i32.const 0
  end
")
	if(WASM_OK)
		set(${OUTVAR} "WASM" PARENT_SCOPE)
		return()
	endif()


	set(${OUTVAR} "UNKNOWN" PARENT_SCOPE)
endfunction()

