/* automatically generated by rust-bindgen */

#[allow(non_snake_case, non_camel_case_types, non_upper_case_globals)]
pub mod root {
    #[repr(C)]
    pub struct __BindgenUnionField<T>(::std::marker::PhantomData<T>);
    impl<T> __BindgenUnionField<T> {
        #[inline]
        pub const fn new() -> Self {
            __BindgenUnionField(::std::marker::PhantomData)
        }
        #[inline]
        pub unsafe fn as_ref(&self) -> &T {
            ::std::mem::transmute(self)
        }
        #[inline]
        pub unsafe fn as_mut(&mut self) -> &mut T {
            ::std::mem::transmute(self)
        }
    }
    impl<T> ::std::default::Default for __BindgenUnionField<T> {
        #[inline]
        fn default() -> Self {
            Self::new()
        }
    }
    impl<T> ::std::clone::Clone for __BindgenUnionField<T> {
        #[inline]
        fn clone(&self) -> Self {
            Self::new()
        }
    }
    impl<T> ::std::marker::Copy for __BindgenUnionField<T> {}
    impl<T> ::std::fmt::Debug for __BindgenUnionField<T> {
        fn fmt(&self, fmt: &mut ::std::fmt::Formatter<'_>) -> ::std::fmt::Result {
            fmt.write_str("__BindgenUnionField")
        }
    }
    impl<T> ::std::hash::Hash for __BindgenUnionField<T> {
        fn hash<H: ::std::hash::Hasher>(&self, _state: &mut H) {}
    }
    impl<T> ::std::cmp::PartialEq for __BindgenUnionField<T> {
        fn eq(&self, _other: &__BindgenUnionField<T>) -> bool {
            true
        }
    }
    impl<T> ::std::cmp::Eq for __BindgenUnionField<T> {}
    #[allow(unused_imports)]
    use self::super::root;
    pub const _LIBCPP_VERSION: u32 = 8000;
    pub const _LIBCPP_ABI_VERSION: u32 = 1;
    pub const _LIBCPP_STD_VER: u32 = 11;
    pub const _LIBCPP_OBJECT_FORMAT_MACHO: u32 = 1;
    pub const _LIBCPP_HIDE_FROM_ABI_PER_TU: u32 = 1;
    pub const _LIBCPP_LOCALE__L_EXTENSIONS: u32 = 1;
    pub const _LIBCPP_HAS_CATOPEN: u32 = 1;
    pub const __WORDSIZE: u32 = 64;
    pub const __DARWIN_ONLY_64_BIT_INO_T: u32 = 0;
    pub const __DARWIN_ONLY_VERS_1050: u32 = 0;
    pub const __DARWIN_ONLY_UNIX_CONFORMANCE: u32 = 1;
    pub const __DARWIN_UNIX03: u32 = 1;
    pub const __DARWIN_64_BIT_INO_T: u32 = 1;
    pub const __DARWIN_VERS_1050: u32 = 1;
    pub const __DARWIN_NON_CANCELABLE: u32 = 0;
    pub const __DARWIN_SUF_64_BIT_INO_T: &'static [u8; 9usize] = b"$INODE64\0";
    pub const __DARWIN_SUF_1050: &'static [u8; 6usize] = b"$1050\0";
    pub const __DARWIN_SUF_EXTSN: &'static [u8; 14usize] = b"$DARWIN_EXTSN\0";
    pub const __DARWIN_C_ANSI: u32 = 4096;
    pub const __DARWIN_C_FULL: u32 = 900000;
    pub const __DARWIN_C_LEVEL: u32 = 900000;
    pub const __DARWIN_NO_LONG_LONG: u32 = 0;
    pub const _DARWIN_FEATURE_64_BIT_INODE: u32 = 1;
    pub const _DARWIN_FEATURE_ONLY_UNIX_CONFORMANCE: u32 = 1;
    pub const _DARWIN_FEATURE_UNIX_CONFORMANCE: u32 = 3;
    pub const __PTHREAD_SIZE__: u32 = 8176;
    pub const __PTHREAD_ATTR_SIZE__: u32 = 56;
    pub const __PTHREAD_MUTEXATTR_SIZE__: u32 = 8;
    pub const __PTHREAD_MUTEX_SIZE__: u32 = 56;
    pub const __PTHREAD_CONDATTR_SIZE__: u32 = 8;
    pub const __PTHREAD_COND_SIZE__: u32 = 40;
    pub const __PTHREAD_ONCE_SIZE__: u32 = 8;
    pub const __PTHREAD_RWLOCK_SIZE__: u32 = 192;
    pub const __PTHREAD_RWLOCKATTR_SIZE__: u32 = 16;
    pub const INT8_MAX: u32 = 127;
    pub const INT16_MAX: u32 = 32767;
    pub const INT32_MAX: u32 = 2147483647;
    pub const INT64_MAX: u64 = 9223372036854775807;
    pub const INT8_MIN: i32 = -128;
    pub const INT16_MIN: i32 = -32768;
    pub const INT32_MIN: i32 = -2147483648;
    pub const INT64_MIN: i64 = -9223372036854775808;
    pub const UINT8_MAX: u32 = 255;
    pub const UINT16_MAX: u32 = 65535;
    pub const UINT32_MAX: u32 = 4294967295;
    pub const UINT64_MAX: i32 = -1;
    pub const INT_LEAST8_MIN: i32 = -128;
    pub const INT_LEAST16_MIN: i32 = -32768;
    pub const INT_LEAST32_MIN: i32 = -2147483648;
    pub const INT_LEAST64_MIN: i64 = -9223372036854775808;
    pub const INT_LEAST8_MAX: u32 = 127;
    pub const INT_LEAST16_MAX: u32 = 32767;
    pub const INT_LEAST32_MAX: u32 = 2147483647;
    pub const INT_LEAST64_MAX: u64 = 9223372036854775807;
    pub const UINT_LEAST8_MAX: u32 = 255;
    pub const UINT_LEAST16_MAX: u32 = 65535;
    pub const UINT_LEAST32_MAX: u32 = 4294967295;
    pub const UINT_LEAST64_MAX: i32 = -1;
    pub const INT_FAST8_MIN: i32 = -128;
    pub const INT_FAST16_MIN: i32 = -32768;
    pub const INT_FAST32_MIN: i32 = -2147483648;
    pub const INT_FAST64_MIN: i64 = -9223372036854775808;
    pub const INT_FAST8_MAX: u32 = 127;
    pub const INT_FAST16_MAX: u32 = 32767;
    pub const INT_FAST32_MAX: u32 = 2147483647;
    pub const INT_FAST64_MAX: u64 = 9223372036854775807;
    pub const UINT_FAST8_MAX: u32 = 255;
    pub const UINT_FAST16_MAX: u32 = 65535;
    pub const UINT_FAST32_MAX: u32 = 4294967295;
    pub const UINT_FAST64_MAX: i32 = -1;
    pub const INTPTR_MAX: u64 = 9223372036854775807;
    pub const INTPTR_MIN: i64 = -9223372036854775808;
    pub const UINTPTR_MAX: i32 = -1;
    pub const SIZE_MAX: i32 = -1;
    pub const WINT_MIN: i32 = -2147483648;
    pub const WINT_MAX: u32 = 2147483647;
    pub const SIG_ATOMIC_MIN: i32 = -2147483648;
    pub const SIG_ATOMIC_MAX: u32 = 2147483647;
    pub mod std {
        #[allow(unused_imports)]
        use self::super::super::root;
    }
    pub type int_least8_t = i8;
    pub type int_least16_t = i16;
    pub type int_least32_t = i32;
    pub type int_least64_t = i64;
    pub type uint_least8_t = u8;
    pub type uint_least16_t = u16;
    pub type uint_least32_t = u32;
    pub type uint_least64_t = u64;
    pub type int_fast8_t = i8;
    pub type int_fast16_t = i16;
    pub type int_fast32_t = i32;
    pub type int_fast64_t = i64;
    pub type uint_fast8_t = u8;
    pub type uint_fast16_t = u16;
    pub type uint_fast32_t = u32;
    pub type uint_fast64_t = u64;
    pub type __int8_t = ::std::os::raw::c_schar;
    pub type __uint8_t = ::std::os::raw::c_uchar;
    pub type __int16_t = ::std::os::raw::c_short;
    pub type __uint16_t = ::std::os::raw::c_ushort;
    pub type __int32_t = ::std::os::raw::c_int;
    pub type __uint32_t = ::std::os::raw::c_uint;
    pub type __int64_t = ::std::os::raw::c_longlong;
    pub type __uint64_t = ::std::os::raw::c_ulonglong;
    pub type __darwin_intptr_t = ::std::os::raw::c_long;
    pub type __darwin_natural_t = ::std::os::raw::c_uint;
    pub type __darwin_ct_rune_t = ::std::os::raw::c_int;
    #[repr(C)]
    pub struct __mbstate_t {
        pub __mbstate8: root::__BindgenUnionField<[::std::os::raw::c_char; 128usize]>,
        pub _mbstateL: root::__BindgenUnionField<::std::os::raw::c_longlong>,
        pub bindgen_union_field: [u64; 16usize],
    }
    pub type __darwin_mbstate_t = root::__mbstate_t;
    pub type __darwin_ptrdiff_t = ::std::os::raw::c_long;
    pub type __darwin_size_t = ::std::os::raw::c_ulong;
    pub type __darwin_va_list = root::__builtin_va_list;
    pub type __darwin_wchar_t = ::std::os::raw::c_int;
    pub type __darwin_rune_t = root::__darwin_wchar_t;
    pub type __darwin_wint_t = ::std::os::raw::c_int;
    pub type __darwin_clock_t = ::std::os::raw::c_ulong;
    pub type __darwin_socklen_t = root::__uint32_t;
    pub type __darwin_ssize_t = ::std::os::raw::c_long;
    pub type __darwin_time_t = ::std::os::raw::c_long;
    pub type __darwin_blkcnt_t = root::__int64_t;
    pub type __darwin_blksize_t = root::__int32_t;
    pub type __darwin_dev_t = root::__int32_t;
    pub type __darwin_fsblkcnt_t = ::std::os::raw::c_uint;
    pub type __darwin_fsfilcnt_t = ::std::os::raw::c_uint;
    pub type __darwin_gid_t = root::__uint32_t;
    pub type __darwin_id_t = root::__uint32_t;
    pub type __darwin_ino64_t = root::__uint64_t;
    pub type __darwin_ino_t = root::__darwin_ino64_t;
    pub type __darwin_mach_port_name_t = root::__darwin_natural_t;
    pub type __darwin_mach_port_t = root::__darwin_mach_port_name_t;
    pub type __darwin_mode_t = root::__uint16_t;
    pub type __darwin_off_t = root::__int64_t;
    pub type __darwin_pid_t = root::__int32_t;
    pub type __darwin_sigset_t = root::__uint32_t;
    pub type __darwin_suseconds_t = root::__int32_t;
    pub type __darwin_uid_t = root::__uint32_t;
    pub type __darwin_useconds_t = root::__uint32_t;
    pub type __darwin_uuid_t = [::std::os::raw::c_uchar; 16usize];
    pub type __darwin_uuid_string_t = [::std::os::raw::c_char; 37usize];
    #[repr(C)]
    #[derive(Debug)]
    pub struct __darwin_pthread_handler_rec {
        pub __routine:
            ::std::option::Option<unsafe extern "C" fn(arg1: *mut ::std::os::raw::c_void)>,
        pub __arg: *mut ::std::os::raw::c_void,
        pub __next: *mut root::__darwin_pthread_handler_rec,
    }
    #[repr(C)]
    pub struct _opaque_pthread_attr_t {
        pub __sig: ::std::os::raw::c_long,
        pub __opaque: [::std::os::raw::c_char; 56usize],
    }
    #[repr(C)]
    pub struct _opaque_pthread_cond_t {
        pub __sig: ::std::os::raw::c_long,
        pub __opaque: [::std::os::raw::c_char; 40usize],
    }
    #[repr(C)]
    #[derive(Debug)]
    pub struct _opaque_pthread_condattr_t {
        pub __sig: ::std::os::raw::c_long,
        pub __opaque: [::std::os::raw::c_char; 8usize],
    }
    #[repr(C)]
    pub struct _opaque_pthread_mutex_t {
        pub __sig: ::std::os::raw::c_long,
        pub __opaque: [::std::os::raw::c_char; 56usize],
    }
    #[repr(C)]
    #[derive(Debug)]
    pub struct _opaque_pthread_mutexattr_t {
        pub __sig: ::std::os::raw::c_long,
        pub __opaque: [::std::os::raw::c_char; 8usize],
    }
    #[repr(C)]
    #[derive(Debug)]
    pub struct _opaque_pthread_once_t {
        pub __sig: ::std::os::raw::c_long,
        pub __opaque: [::std::os::raw::c_char; 8usize],
    }
    #[repr(C)]
    pub struct _opaque_pthread_rwlock_t {
        pub __sig: ::std::os::raw::c_long,
        pub __opaque: [::std::os::raw::c_char; 192usize],
    }
    #[repr(C)]
    #[derive(Debug)]
    pub struct _opaque_pthread_rwlockattr_t {
        pub __sig: ::std::os::raw::c_long,
        pub __opaque: [::std::os::raw::c_char; 16usize],
    }
    #[repr(C)]
    pub struct _opaque_pthread_t {
        pub __sig: ::std::os::raw::c_long,
        pub __cleanup_stack: *mut root::__darwin_pthread_handler_rec,
        pub __opaque: [::std::os::raw::c_char; 8176usize],
    }
    pub type __darwin_pthread_attr_t = root::_opaque_pthread_attr_t;
    pub type __darwin_pthread_cond_t = root::_opaque_pthread_cond_t;
    pub type __darwin_pthread_condattr_t = root::_opaque_pthread_condattr_t;
    pub type __darwin_pthread_key_t = ::std::os::raw::c_ulong;
    pub type __darwin_pthread_mutex_t = root::_opaque_pthread_mutex_t;
    pub type __darwin_pthread_mutexattr_t = root::_opaque_pthread_mutexattr_t;
    pub type __darwin_pthread_once_t = root::_opaque_pthread_once_t;
    pub type __darwin_pthread_rwlock_t = root::_opaque_pthread_rwlock_t;
    pub type __darwin_pthread_rwlockattr_t = root::_opaque_pthread_rwlockattr_t;
    pub type __darwin_pthread_t = *mut root::_opaque_pthread_t;
    pub type u_int8_t = ::std::os::raw::c_uchar;
    pub type u_int16_t = ::std::os::raw::c_ushort;
    pub type u_int32_t = ::std::os::raw::c_uint;
    pub type u_int64_t = ::std::os::raw::c_ulonglong;
    pub type register_t = i64;
    pub type user_addr_t = root::u_int64_t;
    pub type user_size_t = root::u_int64_t;
    pub type user_ssize_t = i64;
    pub type user_long_t = i64;
    pub type user_ulong_t = root::u_int64_t;
    pub type user_time_t = i64;
    pub type user_off_t = i64;
    pub type syscall_arg_t = root::u_int64_t;
    pub type intmax_t = ::std::os::raw::c_long;
    pub type uintmax_t = ::std::os::raw::c_ulong;
    pub mod DB {
        #[allow(unused_imports)]
        use self::super::super::root;
        pub type ConstRawVoidPtr = *const ::std::os::raw::c_void;
        pub type RawVoidPtr = *mut ::std::os::raw::c_void;
        #[repr(u8)]
        #[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
        pub enum ColumnFamilyType {
            Lock = 0,
            Write = 1,
            Default = 2,
        }
        #[repr(C)]
        #[derive(Debug)]
        pub struct RawCppString {
            _unused: [u8; 0],
        }
        pub type RawCppStringPtr = *mut root::DB::RawCppString;
        #[repr(u8)]
        #[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
        pub enum FileEncryptionRes {
            Disabled = 0,
            Ok = 1,
            Error = 2,
        }
        #[repr(u8)]
        #[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
        pub enum EncryptionMethod {
            Unknown = 0,
            Plaintext = 1,
            Aes128Ctr = 2,
            Aes192Ctr = 3,
            Aes256Ctr = 4,
        }
        #[repr(C)]
        #[derive(Debug)]
        pub struct FileEncryptionInfoRaw {
            pub res: root::DB::FileEncryptionRes,
            pub method: root::DB::EncryptionMethod,
            pub key: root::DB::RawCppStringPtr,
            pub iv: root::DB::RawCppStringPtr,
            pub error_msg: root::DB::RawCppStringPtr,
        }
        #[repr(C)]
        #[derive(Debug)]
        pub struct EngineStoreServerWrap {
            _unused: [u8; 0],
        }
        #[repr(u32)]
        #[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
        pub enum EngineStoreApplyRes {
            None = 0,
            Persist = 1,
            NotFound = 2,
        }
        #[repr(u8)]
        #[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
        pub enum WriteCmdType {
            Put = 0,
            Del = 1,
        }
        #[repr(C)]
        #[derive(Debug)]
        pub struct BaseBuffView {
            pub data: *const ::std::os::raw::c_char,
            pub len: u64,
        }
        #[repr(C)]
        #[derive(Debug)]
        pub struct RaftCmdHeader {
            pub region_id: u64,
            pub index: u64,
            pub term: u64,
        }
        #[repr(C)]
        #[derive(Debug)]
        pub struct WriteCmdsView {
            pub keys: *const root::DB::BaseBuffView,
            pub vals: *const root::DB::BaseBuffView,
            pub cmd_types: *const root::DB::WriteCmdType,
            pub cmd_cf: *const root::DB::ColumnFamilyType,
            pub len: u64,
        }
        #[repr(C)]
        #[derive(Debug)]
        pub struct FsStats {
            pub used_size: u64,
            pub avail_size: u64,
            pub capacity_size: u64,
            pub ok: u8,
        }
        #[repr(C)]
        #[derive(Debug)]
        pub struct StoreStats {
            pub fs_stats: root::DB::FsStats,
            pub engine_bytes_written: u64,
            pub engine_keys_written: u64,
            pub engine_bytes_read: u64,
            pub engine_keys_read: u64,
        }
        #[repr(u8)]
        #[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
        pub enum RaftProxyStatus {
            Idle = 0,
            Running = 1,
            Stopped = 2,
        }
        #[repr(u8)]
        #[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
        pub enum EngineStoreServerStatus {
            Idle = 0,
            Running = 1,
            Stopped = 2,
        }
        #[repr(u32)]
        #[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
        pub enum RawCppPtrType {
            None = 0,
            String = 1,
            PreHandledSnapshot = 2,
        }
        #[repr(C)]
        #[derive(Debug)]
        pub struct RawCppPtr {
            pub ptr: root::DB::RawVoidPtr,
            pub type_: root::DB::RawCppPtrType,
        }
        #[repr(C)]
        #[derive(Debug)]
        pub struct CppStrWithView {
            pub inner: root::DB::RawCppPtr,
            pub view: root::DB::BaseBuffView,
        }
        #[repr(u8)]
        #[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
        pub enum HttpRequestStatus {
            Ok = 0,
            ErrorParam = 1,
        }
        #[repr(C)]
        #[derive(Debug)]
        pub struct HttpRequestRes {
            pub status: root::DB::HttpRequestStatus,
            pub res: root::DB::CppStrWithView,
        }
        #[repr(C)]
        #[derive(Debug)]
        pub struct CppStrVecView {
            pub view: *const root::DB::BaseBuffView,
            pub len: u64,
        }
        #[repr(C)]
        #[derive(Debug)]
        pub struct SSTView {
            pub type_: root::DB::ColumnFamilyType,
            pub path: root::DB::BaseBuffView,
        }
        #[repr(C)]
        #[derive(Debug)]
        pub struct SSTViewVec {
            pub views: *const root::DB::SSTView,
            pub len: u64,
        }
        #[repr(C)]
        #[derive(Debug)]
        pub struct RaftStoreProxyPtr {
            pub inner: root::DB::ConstRawVoidPtr,
        }
        #[repr(C)]
        #[derive(Debug)]
        pub struct SSTReaderPtr {
            pub inner: root::DB::RawVoidPtr,
        }
        #[repr(C)]
        #[derive(Debug)]
        pub struct SSTReaderInterfaces {
            pub fn_get_sst_reader: ::std::option::Option<
                unsafe extern "C" fn(
                    arg1: root::DB::SSTView,
                    arg2: root::DB::RaftStoreProxyPtr,
                ) -> root::DB::SSTReaderPtr,
            >,
            pub fn_remained: ::std::option::Option<
                unsafe extern "C" fn(
                    arg1: root::DB::SSTReaderPtr,
                    arg2: root::DB::ColumnFamilyType,
                ) -> u8,
            >,
            pub fn_key: ::std::option::Option<
                unsafe extern "C" fn(
                    arg1: root::DB::SSTReaderPtr,
                    arg2: root::DB::ColumnFamilyType,
                ) -> root::DB::BaseBuffView,
            >,
            pub fn_value: ::std::option::Option<
                unsafe extern "C" fn(
                    arg1: root::DB::SSTReaderPtr,
                    arg2: root::DB::ColumnFamilyType,
                ) -> root::DB::BaseBuffView,
            >,
            pub fn_next: ::std::option::Option<
                unsafe extern "C" fn(
                    arg1: root::DB::SSTReaderPtr,
                    arg2: root::DB::ColumnFamilyType,
                ),
            >,
            pub fn_gc: ::std::option::Option<
                unsafe extern "C" fn(
                    arg1: root::DB::SSTReaderPtr,
                    arg2: root::DB::ColumnFamilyType,
                ),
            >,
        }
        #[repr(C)]
        #[derive(Debug)]
        pub struct RaftStoreProxyFFIHelper {
            pub proxy_ptr: root::DB::RaftStoreProxyPtr,
            pub fn_handle_get_proxy_status: ::std::option::Option<
                unsafe extern "C" fn(
                    arg1: root::DB::RaftStoreProxyPtr,
                ) -> root::DB::RaftProxyStatus,
            >,
            pub fn_is_encryption_enabled: ::std::option::Option<
                unsafe extern "C" fn(arg1: root::DB::RaftStoreProxyPtr) -> u8,
            >,
            pub fn_encryption_method: ::std::option::Option<
                unsafe extern "C" fn(
                    arg1: root::DB::RaftStoreProxyPtr,
                ) -> root::DB::EncryptionMethod,
            >,
            pub fn_handle_get_file: ::std::option::Option<
                unsafe extern "C" fn(
                    arg1: root::DB::RaftStoreProxyPtr,
                    arg2: root::DB::BaseBuffView,
                ) -> root::DB::FileEncryptionInfoRaw,
            >,
            pub fn_handle_new_file: ::std::option::Option<
                unsafe extern "C" fn(
                    arg1: root::DB::RaftStoreProxyPtr,
                    arg2: root::DB::BaseBuffView,
                ) -> root::DB::FileEncryptionInfoRaw,
            >,
            pub fn_handle_delete_file: ::std::option::Option<
                unsafe extern "C" fn(
                    arg1: root::DB::RaftStoreProxyPtr,
                    arg2: root::DB::BaseBuffView,
                ) -> root::DB::FileEncryptionInfoRaw,
            >,
            pub fn_handle_link_file: ::std::option::Option<
                unsafe extern "C" fn(
                    arg1: root::DB::RaftStoreProxyPtr,
                    arg2: root::DB::BaseBuffView,
                    arg3: root::DB::BaseBuffView,
                ) -> root::DB::FileEncryptionInfoRaw,
            >,
            pub fn_handle_batch_read_index: ::std::option::Option<
                unsafe extern "C" fn(
                    arg1: root::DB::RaftStoreProxyPtr,
                    arg2: root::DB::CppStrVecView,
                ) -> root::DB::RawVoidPtr,
            >,
            pub sst_reader_interfaces: root::DB::SSTReaderInterfaces,
        }
        #[repr(C)]
        #[derive(Debug)]
        pub struct EngineStoreServerHelper {
            pub magic_number: u32,
            pub version: u32,
            pub inner: *mut root::DB::EngineStoreServerWrap,
            pub fn_gen_cpp_string: ::std::option::Option<
                unsafe extern "C" fn(arg1: root::DB::BaseBuffView) -> root::DB::RawCppPtr,
            >,
            pub fn_handle_write_raft_cmd: ::std::option::Option<
                unsafe extern "C" fn(
                    arg1: *const root::DB::EngineStoreServerWrap,
                    arg2: root::DB::WriteCmdsView,
                    arg3: root::DB::RaftCmdHeader,
                ) -> root::DB::EngineStoreApplyRes,
            >,
            pub fn_handle_admin_raft_cmd: ::std::option::Option<
                unsafe extern "C" fn(
                    arg1: *const root::DB::EngineStoreServerWrap,
                    arg2: root::DB::BaseBuffView,
                    arg3: root::DB::BaseBuffView,
                    arg4: root::DB::RaftCmdHeader,
                ) -> root::DB::EngineStoreApplyRes,
            >,
            pub fn_atomic_update_proxy: ::std::option::Option<
                unsafe extern "C" fn(
                    arg1: *mut root::DB::EngineStoreServerWrap,
                    arg2: *mut root::DB::RaftStoreProxyFFIHelper,
                ),
            >,
            pub fn_handle_destroy: ::std::option::Option<
                unsafe extern "C" fn(arg1: *mut root::DB::EngineStoreServerWrap, arg2: u64),
            >,
            pub fn_handle_ingest_sst: ::std::option::Option<
                unsafe extern "C" fn(
                    arg1: *mut root::DB::EngineStoreServerWrap,
                    arg2: root::DB::SSTViewVec,
                    arg3: root::DB::RaftCmdHeader,
                ) -> root::DB::EngineStoreApplyRes,
            >,
            pub fn_handle_check_terminated: ::std::option::Option<
                unsafe extern "C" fn(arg1: *mut root::DB::EngineStoreServerWrap) -> u8,
            >,
            pub fn_handle_compute_store_stats: ::std::option::Option<
                unsafe extern "C" fn(
                    arg1: *mut root::DB::EngineStoreServerWrap,
                ) -> root::DB::StoreStats,
            >,
            pub fn_handle_get_engine_store_server_status: ::std::option::Option<
                unsafe extern "C" fn(
                    arg1: *mut root::DB::EngineStoreServerWrap,
                ) -> root::DB::EngineStoreServerStatus,
            >,
            pub fn_pre_handle_snapshot: ::std::option::Option<
                unsafe extern "C" fn(
                    arg1: *mut root::DB::EngineStoreServerWrap,
                    arg2: root::DB::BaseBuffView,
                    arg3: u64,
                    arg4: root::DB::SSTViewVec,
                    arg5: u64,
                    arg6: u64,
                ) -> root::DB::RawCppPtr,
            >,
            pub fn_apply_pre_handled_snapshot: ::std::option::Option<
                unsafe extern "C" fn(
                    arg1: *mut root::DB::EngineStoreServerWrap,
                    arg2: root::DB::RawVoidPtr,
                    arg3: root::DB::RawCppPtrType,
                ),
            >,
            pub fn_handle_http_request: ::std::option::Option<
                unsafe extern "C" fn(
                    arg1: *mut root::DB::EngineStoreServerWrap,
                    arg2: root::DB::BaseBuffView,
                ) -> root::DB::HttpRequestRes,
            >,
            pub fn_check_http_uri_available:
                ::std::option::Option<unsafe extern "C" fn(arg1: root::DB::BaseBuffView) -> u8>,
            pub fn_gc_raw_cpp_ptr: ::std::option::Option<
                unsafe extern "C" fn(
                    arg1: *mut root::DB::EngineStoreServerWrap,
                    arg2: root::DB::RawVoidPtr,
                    arg3: root::DB::RawCppPtrType,
                ),
            >,
            pub fn_gen_batch_read_index_res:
                ::std::option::Option<unsafe extern "C" fn(arg1: u64) -> root::DB::RawVoidPtr>,
            pub fn_insert_batch_read_index_resp: ::std::option::Option<
                unsafe extern "C" fn(
                    arg1: root::DB::RawVoidPtr,
                    arg2: root::DB::BaseBuffView,
                    arg3: u64,
                ),
            >,
        }
        pub const RAFT_STORE_PROXY_VERSION: u32 = 400004;
        pub const RAFT_STORE_PROXY_MAGIC_NUMBER: u32 = 324508639;
    }
    pub type __builtin_va_list = [root::__va_list_tag; 1usize];
    #[repr(C)]
    #[derive(Debug)]
    pub struct __va_list_tag {
        pub gp_offset: ::std::os::raw::c_uint,
        pub fp_offset: ::std::os::raw::c_uint,
        pub overflow_arg_area: *mut ::std::os::raw::c_void,
        pub reg_save_area: *mut ::std::os::raw::c_void,
    }
}
