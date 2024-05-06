// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <string>

#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/rust/accessors/accessor_case.h"
#include "google/protobuf/compiler/rust/accessors/accessor_generator.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

void RepeatedField::InMsgImpl(Context& ctx, const FieldDescriptor& field,
                              AccessorCase accessor_case) const {
  std::string field_name = FieldNameWithCollisionAvoidance(field);
  ctx.Emit(
      {
          {"field", RsSafeName(field_name)},
          {"raw_field_name", field_name},  // Never r# prefixed
          {"RsType", RsTypePath(ctx, field)},
          {"view_lifetime", ViewLifetime(accessor_case)},
          {"view_self", ViewReceiver(accessor_case)},
          {"getter_thunk", ThunkName(ctx, field, "get")},
          {"getter_mut_thunk", ThunkName(ctx, field, "get_mut")},
          {"getter",
           [&] {
             if (ctx.is_upb()) {
               ctx.Emit({}, R"rs(
                    pub fn $field$($view_self$) -> $pb$::RepeatedView<$view_lifetime$, $RsType$> {
                      unsafe {
                        $getter_thunk$(
                          self.raw_msg(),
                          /* optional size pointer */ std::ptr::null(),
                        ) }
                        .map_or_else(
                          $pbr$::empty_array::<$RsType$>,
                          |raw| unsafe {
                            $pb$::RepeatedView::from_raw($pbi$::Private, raw)
                          }
                        )
                    }
                  )rs");
             } else {
               ctx.Emit({}, R"rs(
                    pub fn $field$($view_self$) -> $pb$::RepeatedView<$view_lifetime$, $RsType$> {
                      unsafe {
                        $pb$::RepeatedView::from_raw(
                          $pbi$::Private,
                          $getter_thunk$(self.raw_msg()),
                        )
                      }
                    }
                  )rs");
             }
           }},
          {"clearer_thunk", ThunkName(ctx, field, "clear")},
          {"getter_mut",
           [&] {
             if (accessor_case == AccessorCase::VIEW) {
               return;
             }
             if (ctx.is_upb()) {
               ctx.Emit({}, R"rs(
                    pub fn $field$_mut(&mut self) -> $pb$::RepeatedMut<'_, $RsType$> {
                      unsafe {
                        $pb$::RepeatedMut::from_inner(
                          $pbi$::Private,
                          $pbr$::InnerRepeatedMut::new(
                            $pbi$::Private,
                            $getter_mut_thunk$(
                              self.raw_msg(),
                              /* optional size pointer */ std::ptr::null(),
                              self.arena().raw(),
                            ),
                            self.arena(),
                          ),
                        )
                      }
                    }
                  )rs");
             } else {
               ctx.Emit({}, R"rs(
                      pub fn $field$_mut(&mut self) -> $pb$::RepeatedMut<'_, $RsType$> {
                        unsafe {
                          $pb$::RepeatedMut::from_inner(
                            $pbi$::Private,
                            $pbr$::InnerRepeatedMut::new(
                              $pbi$::Private,
                              $getter_mut_thunk$(self.raw_msg()),
                            ),
                          )
                        }
                      }
                    )rs");
             }
           }},
          {"move_setter_thunk", ThunkName(ctx, field, "move_set")},
          {"setter",
           [&] {
             if (accessor_case == AccessorCase::VIEW) {
               return;
             }
             if (ctx.is_upb()) {
               ctx.Emit({{"field_number", field.number()}}, R"rs(
                    pub fn set_$raw_field_name$(&mut self, src: impl $pb$::IntoProxied<$pb$::Repeated<$RsType$>>) {
                      let minitable_field = unsafe {
                        $pbr$::upb_MiniTable_FindFieldByNumber(
                          Self::raw_minitable($pbi$::Private),
                          $field_number$
                        )
                      };
                      let val = src.into($pbi$::Private);
                      let inner = val.inner($pbi$::Private);

                      self.arena().fuse(inner.arena());
                      unsafe {
                          let value_ptr: *const *const std::ffi::c_void =
                              &(inner.raw().as_ptr() as *const std::ffi::c_void);
                          $pbr$::upb_Message_SetBaseField(self.raw_msg(),
                            minitable_field,
                            value_ptr as *const std::ffi::c_void);
                      }
                    }
                  )rs");
             } else {
               ctx.Emit({}, R"rs(
                      pub fn set_$raw_field_name$(&mut self, src: impl $pb$::IntoProxied<$pb$::Repeated<$RsType$>>) {
                        // Prevent the memory from being deallocated. The setter
                        // transfers ownership of the memory to the parent message.
                        let val = std::mem::ManuallyDrop::new(src.into($pbi$::Private));
                        unsafe {
                          $move_setter_thunk$(self.raw_msg(),
                            val.inner($pbi$::Private).raw());
                        }
                      }
                    )rs");
             }
           }},
      },
      R"rs(
          $getter$
          $getter_mut$
          $setter$
        )rs");
}

void RepeatedField::InExternC(Context& ctx,
                              const FieldDescriptor& field) const {
  ctx.Emit({{"getter_thunk", ThunkName(ctx, field, "get")},
            {"getter_mut_thunk", ThunkName(ctx, field, "get_mut")},
            {"move_setter_thunk", ThunkName(ctx, field, "move_set")},
            {"getter",
             [&] {
               if (ctx.is_upb()) {
                 ctx.Emit(R"rs(
                    fn $getter_mut_thunk$(
                      raw_msg: $pbr$::RawMessage,
                      size: *const usize,
                      arena: $pbr$::RawArena,
                    ) -> $pbr$::RawRepeatedField;
                    //  Returns `None` when returned array pointer is NULL.
                    fn $getter_thunk$(
                      raw_msg: $pbr$::RawMessage,
                      size: *const usize,
                    ) -> Option<$pbr$::RawRepeatedField>;
                  )rs");
               } else {
                 ctx.Emit(R"rs(
                    fn $getter_mut_thunk$(raw_msg: $pbr$::RawMessage) -> $pbr$::RawRepeatedField;
                    fn $getter_thunk$(raw_msg: $pbr$::RawMessage) -> $pbr$::RawRepeatedField;
                    fn $move_setter_thunk$(raw_msg: $pbr$::RawMessage, value: $pbr$::RawRepeatedField);
                  )rs");
               }
             }},
            {"clearer_thunk", ThunkName(ctx, field, "clear")}},
           R"rs(
          fn $clearer_thunk$(raw_msg: $pbr$::RawMessage);
          $getter$
        )rs");
}

bool IsRepeatedPrimitive(const FieldDescriptor& field) {
  return field.cpp_type() == FieldDescriptor::CPPTYPE_ENUM ||
         field.cpp_type() == FieldDescriptor::CPPTYPE_BOOL ||
         field.cpp_type() == FieldDescriptor::CPPTYPE_DOUBLE ||
         field.cpp_type() == FieldDescriptor::CPPTYPE_FLOAT ||
         field.cpp_type() == FieldDescriptor::CPPTYPE_INT32 ||
         field.cpp_type() == FieldDescriptor::CPPTYPE_INT64 ||
         field.cpp_type() == FieldDescriptor::CPPTYPE_UINT32 ||
         field.cpp_type() == FieldDescriptor::CPPTYPE_UINT64;
}

bool IsRepeatedPtrPrimitive(const FieldDescriptor& field) {
  return field.cpp_type() == FieldDescriptor::CPPTYPE_STRING;
}

std::string CppElementType(const FieldDescriptor& field) {
  if (IsRepeatedPrimitive(field) || IsRepeatedPtrPrimitive(field)) {
    return cpp::PrimitiveTypeName(field.cpp_type());
  } else {
    return cpp::QualifiedClassName(field.message_type());
  }
}

const char* CppRepeatedContainerType(const FieldDescriptor& field) {
  if (IsRepeatedPrimitive(field)) {
    return "google::protobuf::RepeatedField";
  } else {
    return "google::protobuf::RepeatedPtrField";
  }
}

void RepeatedField::InThunkCc(Context& ctx,
                              const FieldDescriptor& field) const {
  ctx.Emit({{"field", cpp::FieldName(&field)},
            {"ElementType", CppElementType(field)},
            {"ContainerType", CppRepeatedContainerType(field)},
            {"QualifiedMsg", cpp::QualifiedClassName(field.containing_type())},
            {"clearer_thunk", ThunkName(ctx, field, "clear")},
            {"getter_thunk", ThunkName(ctx, field, "get")},
            {"getter_mut_thunk", ThunkName(ctx, field, "get_mut")},
            {"repeated_copy_from_thunk",
             ThunkName(ctx, field, "repeated_copy_from")},
            {"move_setter_thunk", ThunkName(ctx, field, "move_set")},
            {"impls",
             [&] {
               ctx.Emit(
                   R"cc(
                     void $clearer_thunk$($QualifiedMsg$* msg) {
                       msg->clear_$field$();
                     }
                     $ContainerType$<$ElementType$>* $getter_mut_thunk$($QualifiedMsg$* msg) {
                       return msg->mutable_$field$();
                     }
                     const $ContainerType$<$ElementType$>* $getter_thunk$(
                         const $QualifiedMsg$* msg) {
                       return &msg->$field$();
                     }
                     void $move_setter_thunk$(
                         $QualifiedMsg$* msg,
                         $ContainerType$<$ElementType$>* value) {
                       *msg->mutable_$field$() = std::move(*value);
                     }
                   )cc");
             }}},
           "$impls$");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
