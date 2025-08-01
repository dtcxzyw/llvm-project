; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc -mtriple=amdgcn < %s | FileCheck -check-prefix=GCN %s

define amdgpu_kernel void @negated_cond(ptr addrspace(1) %arg1) {
; GCN-LABEL: negated_cond:
; GCN:       ; %bb.0: ; %bb
; GCN-NEXT:    s_load_dwordx2 s[4:5], s[4:5], 0x9
; GCN-NEXT:    s_mov_b32 s7, 0xf000
; GCN-NEXT:    s_mov_b32 s10, -1
; GCN-NEXT:    s_mov_b32 s6, 0
; GCN-NEXT:    s_mov_b32 s11, s7
; GCN-NEXT:    s_waitcnt lgkmcnt(0)
; GCN-NEXT:    s_mov_b32 s8, s4
; GCN-NEXT:    s_mov_b32 s9, s5
; GCN-NEXT:    v_mov_b32_e32 v0, 0
; GCN-NEXT:    s_branch .LBB0_2
; GCN-NEXT:  .LBB0_1: ; %loop.exit.guard
; GCN-NEXT:    ; in Loop: Header=BB0_2 Depth=1
; GCN-NEXT:    s_and_b64 vcc, exec, s[14:15]
; GCN-NEXT:    s_cbranch_vccnz .LBB0_9
; GCN-NEXT:  .LBB0_2: ; %bb1
; GCN-NEXT:    ; =>This Loop Header: Depth=1
; GCN-NEXT:    ; Child Loop BB0_4 Depth 2
; GCN-NEXT:    buffer_load_dword v1, off, s[8:11], 0
; GCN-NEXT:    s_waitcnt vmcnt(0)
; GCN-NEXT:    v_cmp_ne_u32_e64 s[2:3], 0, v1
; GCN-NEXT:    v_cmp_eq_u32_e32 vcc, 0, v1
; GCN-NEXT:    v_cndmask_b32_e64 v1, 0, 1, vcc
; GCN-NEXT:    v_cmp_ne_u32_e64 s[0:1], 1, v1
; GCN-NEXT:    s_mov_b32 s12, s6
; GCN-NEXT:    s_branch .LBB0_4
; GCN-NEXT:  .LBB0_3: ; %Flow1
; GCN-NEXT:    ; in Loop: Header=BB0_4 Depth=2
; GCN-NEXT:    s_andn2_b64 vcc, exec, s[16:17]
; GCN-NEXT:    s_cbranch_vccz .LBB0_1
; GCN-NEXT:  .LBB0_4: ; %bb2
; GCN-NEXT:    ; Parent Loop BB0_2 Depth=1
; GCN-NEXT:    ; => This Inner Loop Header: Depth=2
; GCN-NEXT:    s_and_b64 vcc, exec, s[0:1]
; GCN-NEXT:    s_lshl_b32 s12, s12, 5
; GCN-NEXT:    s_cbranch_vccz .LBB0_6
; GCN-NEXT:  ; %bb.5: ; in Loop: Header=BB0_4 Depth=2
; GCN-NEXT:    s_mov_b64 s[16:17], s[2:3]
; GCN-NEXT:    s_branch .LBB0_7
; GCN-NEXT:  .LBB0_6: ; %bb3
; GCN-NEXT:    ; in Loop: Header=BB0_4 Depth=2
; GCN-NEXT:    s_add_i32 s12, s12, 1
; GCN-NEXT:    s_mov_b64 s[16:17], -1
; GCN-NEXT:  .LBB0_7: ; %Flow
; GCN-NEXT:    ; in Loop: Header=BB0_4 Depth=2
; GCN-NEXT:    s_mov_b64 s[14:15], -1
; GCN-NEXT:    s_andn2_b64 vcc, exec, s[16:17]
; GCN-NEXT:    s_mov_b64 s[16:17], -1
; GCN-NEXT:    s_cbranch_vccnz .LBB0_3
; GCN-NEXT:  ; %bb.8: ; %bb4
; GCN-NEXT:    ; in Loop: Header=BB0_4 Depth=2
; GCN-NEXT:    s_ashr_i32 s13, s12, 31
; GCN-NEXT:    s_lshl_b64 s[16:17], s[12:13], 2
; GCN-NEXT:    s_mov_b64 s[14:15], 0
; GCN-NEXT:    v_mov_b32_e32 v1, s16
; GCN-NEXT:    v_mov_b32_e32 v2, s17
; GCN-NEXT:    buffer_store_dword v0, v[1:2], s[4:7], 0 addr64
; GCN-NEXT:    s_cmp_eq_u32 s12, 32
; GCN-NEXT:    s_cselect_b64 s[16:17], -1, 0
; GCN-NEXT:    s_branch .LBB0_3
; GCN-NEXT:  .LBB0_9: ; %DummyReturnBlock
; GCN-NEXT:    s_endpgm
bb:
  br label %bb1

bb1:
  %tmp1 = load i32, ptr addrspace(1) %arg1
  %tmp2 = icmp eq i32 %tmp1, 0
  br label %bb2

bb2:
  %tmp3 = phi i32 [ 0, %bb1 ], [ %tmp6, %bb4 ]
  %tmp4 = shl i32 %tmp3, 5
  br i1 %tmp2, label %bb3, label %bb4

bb3:
  %tmp5 = add i32 %tmp4, 1
  br label %bb4

bb4:
  %tmp6 = phi i32 [ %tmp5, %bb3 ], [ %tmp4, %bb2 ]
  %gep = getelementptr inbounds i32, ptr addrspace(1) %arg1, i32 %tmp6
  store i32 0, ptr addrspace(1) %gep
  %tmp7 = icmp eq i32 %tmp6, 32
  br i1 %tmp7, label %bb1, label %bb2
}

define amdgpu_kernel void @negated_cond_dominated_blocks(ptr addrspace(1) %arg1) {
; GCN-LABEL: negated_cond_dominated_blocks:
; GCN:       ; %bb.0: ; %bb
; GCN-NEXT:    s_load_dwordx2 s[4:5], s[4:5], 0x9
; GCN-NEXT:    s_waitcnt lgkmcnt(0)
; GCN-NEXT:    s_load_dword s0, s[4:5], 0x0
; GCN-NEXT:    s_mov_b32 s6, 0
; GCN-NEXT:    s_mov_b32 s7, 0xf000
; GCN-NEXT:    s_waitcnt lgkmcnt(0)
; GCN-NEXT:    s_cmp_lg_u32 s0, 0
; GCN-NEXT:    s_cselect_b64 s[0:1], -1, 0
; GCN-NEXT:    s_and_b64 s[0:1], exec, s[0:1]
; GCN-NEXT:    v_mov_b32_e32 v0, 0
; GCN-NEXT:    s_mov_b32 s3, s6
; GCN-NEXT:    s_branch .LBB1_2
; GCN-NEXT:  .LBB1_1: ; %bb7
; GCN-NEXT:    ; in Loop: Header=BB1_2 Depth=1
; GCN-NEXT:    s_ashr_i32 s3, s2, 31
; GCN-NEXT:    s_lshl_b64 s[8:9], s[2:3], 2
; GCN-NEXT:    v_mov_b32_e32 v1, s8
; GCN-NEXT:    v_mov_b32_e32 v2, s9
; GCN-NEXT:    s_cmp_eq_u32 s2, 32
; GCN-NEXT:    buffer_store_dword v0, v[1:2], s[4:7], 0 addr64
; GCN-NEXT:    s_mov_b32 s3, s2
; GCN-NEXT:    s_cbranch_scc1 .LBB1_6
; GCN-NEXT:  .LBB1_2: ; %bb4
; GCN-NEXT:    ; =>This Inner Loop Header: Depth=1
; GCN-NEXT:    s_mov_b64 vcc, s[0:1]
; GCN-NEXT:    s_cbranch_vccz .LBB1_4
; GCN-NEXT:  ; %bb.3: ; %bb6
; GCN-NEXT:    ; in Loop: Header=BB1_2 Depth=1
; GCN-NEXT:    s_add_i32 s2, s3, 1
; GCN-NEXT:    s_mov_b64 vcc, exec
; GCN-NEXT:    s_cbranch_execnz .LBB1_1
; GCN-NEXT:    s_branch .LBB1_5
; GCN-NEXT:  .LBB1_4: ; in Loop: Header=BB1_2 Depth=1
; GCN-NEXT:    ; implicit-def: $sgpr2
; GCN-NEXT:    s_mov_b64 vcc, 0
; GCN-NEXT:  .LBB1_5: ; %bb5
; GCN-NEXT:    ; in Loop: Header=BB1_2 Depth=1
; GCN-NEXT:    s_lshl_b32 s2, s3, 5
; GCN-NEXT:    s_or_b32 s2, s2, 1
; GCN-NEXT:    s_branch .LBB1_1
; GCN-NEXT:  .LBB1_6: ; %bb3
; GCN-NEXT:    s_endpgm
bb:
  br label %bb2

bb2:
  %tmp1 = load i32, ptr addrspace(1) %arg1
  %tmp2 = icmp eq i32 %tmp1, 0
  br label %bb4

bb3:
  ret void

bb4:
  %tmp3 = phi i32 [ 0, %bb2 ], [ %tmp7, %bb7 ]
  %tmp4 = shl i32 %tmp3, 5
  br i1 %tmp2, label %bb5, label %bb6

bb5:
  %tmp5 = add i32 %tmp4, 1
  br label %bb7

bb6:
  %tmp6 = add i32 %tmp3, 1
  br label %bb7

bb7:
  %tmp7 = phi i32 [ %tmp5, %bb5 ], [ %tmp6, %bb6 ]
  %gep = getelementptr inbounds i32, ptr addrspace(1) %arg1, i32 %tmp7
  store i32 0, ptr addrspace(1) %gep
  %tmp8 = icmp eq i32 %tmp7, 32
  br i1 %tmp8, label %bb3, label %bb4
}
