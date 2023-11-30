# BufferCache

simple buffer cache

## Todo

- Dirty Buffer를 Disk에 Write하는 기능을 Thread (flush thread)를 이용하여
  해볼 것
  - Buffer가 flush thread에 의해 Disk에 Write 중인 경우, 응용이 overwrite하지 못하도록
    동기화 (locking) 메커니즘을 사용할 것
- ~~FIFO,~~ LRU, LFU 등 널리 알려진 알고리즘을 3개 이상 구현한다.
- Normal distribution 또는 Zipfian Distribution을 따르는 Block Access
  Sequence를 생성하고, 구현한 Replacement Policy에서 Hit가 얼마나
  발생하는 지 테스트





## Reference

- hash table
  - https://github.com/goldsborough/hashtable
- queue
  - https://gist.github.com/rdleon/d569a219c6144c4dfc04366fd6298554
- linked list
  - https://gist.github.com/echo-akash/b1345925b6c801217f7cde452f8e2c73