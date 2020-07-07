use crate::tiflash_ffi::WriteCmdType;
use engine::rocks::{DBValueType, WriteBatchIter, WriteBatchRef};

pub struct RocksWriteBatchIter<'b> {
    inner: WriteBatchIter<'b>,
}

impl<'a, 'b> Iterator for RocksWriteBatchIter<'b> {
    type Item = (WriteCmdType, &'static str, &'b [u8], &'b [u8]);

    fn next(&mut self) -> Option<(WriteCmdType, &'static str, &'b [u8], &'b [u8])> {
        self.inner.next().map(|(t, cf_id, key, value)| {
            let value_type = match t {
                DBValueType::TypeDeletion => WriteCmdType::Del,
                DBValueType::TypeValue => WriteCmdType::Put,
                _ => WriteCmdType::None,
            };
            let cf = engine_traits::DATA_CFS[cf_id as usize];
            (value_type, cf, key, value)
        })
    }
}

#[derive(Clone)]
pub struct RocksWriteBatchReader {}

impl RocksWriteBatchReader {
    pub fn iter(data: &[u8]) -> RocksWriteBatchIter {
        let wb = WriteBatchRef::new(data);
        let inner = wb.iter();
        RocksWriteBatchIter { inner }
    }

    pub fn count(data: &[u8]) -> usize {
        let wb = WriteBatchRef::new(data);
        wb.count()
    }
}
