/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ecma-arraybuffer-object.h"
#include "ecma-try-catch-macro.h"
#include "ecma-objects.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "jmem.h"

#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaarraybufferobject ECMA ArrayBuffer object related routines
 * @{
 */

/**
 * ArrayBuffer object creation operation.
 *
 * See also: ES2015 24.1.1.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_create_arraybuffer_object (const ecma_value_t *arguments_list_p, /**< list of arguments that
                                                                          *   are passed to String constructor */
                                   ecma_length_t arguments_list_len) /**< length of the arguments' list */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  uint32_t length = 0;

  if (arguments_list_len > 0)
  {
    ecma_value_t ret = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

    ECMA_OP_TO_NUMBER_TRY_CATCH (num, arguments_list_p[0], ret);
    length = ecma_number_to_uint32 (num);

    if (num != ((ecma_number_t) length))
    {
      return ecma_raise_range_error (ECMA_ERR_MSG ("Invalid ArrayBuffer length."));
    }

    ECMA_OP_TO_NUMBER_FINALIZE (num);

    if (!ecma_is_value_empty (ret))
    {
      return ret;
    }
  }

  if (length > UINT32_MAX - sizeof (ecma_extended_object_t) - JMEM_ALIGNMENT + 1)
  {
    return ecma_raise_range_error (ECMA_ERR_MSG ("Invalid ArrayBuffer length."));
  }

  ecma_object_t *object_p = ecma_arraybuffer_new_object (length);

  return ecma_make_object_value (object_p);
} /* ecma_op_create_arraybuffer_object */

/**
 * Helper function: create arraybuffer object based on the array length
 *
 * The struct of arraybuffer object:
 *   ecma_object_t
 *   extend_part
 *   data buffer
 *
 * @return ecma_object_t *
 */
ecma_object_t *
ecma_arraybuffer_new_object (ecma_length_t length) /**< length of the arraybuffer */
{
  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_ARRAYBUFFER_PROTOTYPE);
  ecma_object_t *object_p = ecma_create_object (prototype_obj_p,
                                                sizeof (ecma_extended_object_t) + length,
                                                ECMA_OBJECT_TYPE_CLASS);
  ecma_deref_object (prototype_obj_p);
  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
  ext_object_p->u.class_prop.class_id = LIT_MAGIC_STRING_ARRAY_BUFFER_UL;
  ext_object_p->u.class_prop.u.length = length;

  lit_utf8_byte_t *buf = (lit_utf8_byte_t *) (ext_object_p + 1);
  memset (buf, 0, length);

  return object_p;
} /* ecma_arraybuffer_new_object */

/**
 * Helper function: create arraybuffer object by cloning another arraybuffer
 *
 * See also: ES2015 24.1.1.4
 *
 * @return cloned array buffer
 */
ecma_object_t *
ecma_arraybuffer_clone_arraybuffer (ecma_object_t *array_buf_p, /**< the cloned array buffer  */
                                    ecma_length_t offset) /**< start offset */
{

  JERRY_ASSERT (offset <= ecma_arraybuffer_get_length (array_buf_p));

  ecma_length_t length = ecma_arraybuffer_get_length (array_buf_p) - offset;
  ecma_object_t *new_array_buf_p = ecma_arraybuffer_new_object (length);
  lit_utf8_byte_t *src_buf_p = ecma_arraybuffer_get_buffer (array_buf_p);
  lit_utf8_byte_t *dst_buf_p = ecma_arraybuffer_get_buffer (new_array_buf_p);
  memcpy (dst_buf_p, src_buf_p + offset, length);

  return new_array_buf_p;
} /* ecma_arraybuffer_clone_arraybuffer */

/**
 * Helper function: check if the target is ArrayBuffer
 *
 *
 * See also: ES2015 24.1.1.4
 *
 * @return true - if value is an ArrayBuffer object
 *         false - otherwise
 */
bool
ecma_is_arraybuffer (ecma_value_t target) /**< the target value */
{
  if (!ecma_is_value_object (target))
  {
    return false;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (target);
  lit_magic_string_id_t class_id = ecma_object_get_class_name (obj_p);

  return class_id == LIT_MAGIC_STRING_ARRAY_BUFFER_UL;
} /* ecma_is_arraybuffer */

/**
 * Helper function: return the length of the buffer inside the arraybuffer object
 *
 * @return ecma_length_t, the length of the arraybuffer
 */
ecma_length_t __attr_pure___
ecma_arraybuffer_get_length (ecma_object_t *object_p) /**< pointer to the ArrayBuffer object */
{
  JERRY_ASSERT (ecma_object_class_is (object_p, LIT_MAGIC_STRING_ARRAY_BUFFER_UL));

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
  return ext_object_p->u.class_prop.u.length;
} /* ecma_arraybuffer_get_length */

/**
 * Helper function: return the pointer to the data buffer inside the arraybuffer object
 *
 * @return pointer to the data buffer
 */
inline lit_utf8_byte_t * __attr_pure___ __attr_always_inline___
ecma_arraybuffer_get_buffer (ecma_object_t *object_p) /**< pointer to the ArrayBuffer object */
{
  JERRY_ASSERT (ecma_object_class_is (object_p, LIT_MAGIC_STRING_ARRAY_BUFFER_UL));

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
  return (lit_utf8_byte_t *) (ext_object_p + 1);
} /* ecma_arraybuffer_get_buffer */

/**
 * @}
 * @}
 */
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
