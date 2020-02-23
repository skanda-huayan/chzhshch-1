  class CharacterVec: public IComparable
	{
	public:
		// 特征向量； 如果，特征向量只有1笔，那么start和end指向同一个笔，否则，分别指向第一、最后一笔；
		baseItemIterator start, end;

		float getHigh() const {return High;}
		float getLow() const {return Low;}
		void setHigh(float h) {High = h;}
		void setLow(float l) {Low = l;}

		CharacterVec(baseItemIterator &s, baseItemIterator &e) 
		{
			assert((*s).getDirection() == (*e).getDirection());
			start = s; 
			end = e;
			d = (*s).getDirection();
			
			High = (*s).getHigh();
			Low = (*s).getLow();

			while (s != e)
			{
				s += 2;
				merge(*s, -d);
			}
		}

	private:
		Direction d;
		float High, Low;	
	};

	typedef vector<CharacterVec> analyzeStack;

	static baseItemIterator FXD_Merge(Direction hints, const baseItemIterator & start, const baseItemIterator & end, baseItemIterator &prevCharacVecEnd)
	{
		assert(hints != ENCLOSING);

		baseItemIterator current = start;
		Direction d;
	
		/* 先合并（可能的）新线段开始处的包含（前包后）的几笔 */
		baseItemType possiblePrevXianDuanChracVec = *current;
		while (current < end - 2)
		{
			baseItemType& suspect = *(current + 2);
			if (possiblePrevXianDuanChracVec >> suspect)
			{
				possiblePrevXianDuanChracVec = suspect;
				current += 2;
				continue;
			}
			else
				break;
		}

		prevCharacVecEnd = current;

		/* 找可能出现转折的位置 */
		baseItemType lastBi = possiblePrevXianDuanChracVec;
		while (current < end - 2)
		{
			baseItemType &suspect = *(current + 2);
			d = IComparable::getDirection(lastBi, suspect);
			if (d == hints || (lastBi << suspect))
			{
				lastBi = suspect;
				current += 2;
				continue;
			}
			else
				break;
		}

		return current;
	}

	static bool FXD_Case1(Direction hints, baseItemIterator &start, const baseItemIterator &end, analyzeStack& lastXianDuan_CharacVecStack)
	{
		baseItemIterator prevChacVecEnd;  
		baseItemIterator possibleNewXianDuanEnd = FXD_Merge(hints, start, end, prevChacVecEnd);

		
		/*  算上合并包含关系得到的那一笔， 到可能出现转折的笔，一共（至少）3笔，那么新线段就算成立，即：3笔确立线段*/
		if (possibleNewXianDuanEnd - prevChacVecEnd >= 2) 
			return true;
		else
		{
			/* 否则，处理合并关系得到的这一笔，被当做原线段的特征向量，原线段仍然继续 */
			lastXianDuan_CharacVecStack.push_back(CharacterVec(start, prevChacVecEnd));
			return false;
		}
	}


	static void Normalize(Direction hints, baseItemIterator start, baseItemIterator end)
	{
		if (hints == (*start).getDirection()) return;

		baseItemIterator current = start;

		/* 所有线段方向均取反，先处理序号是奇数的线段 */
		while (current != end)
		{
			(*current).d = -(*current).d;

			current++;
			if (current == end)
				break;
			else
				current++;
		}

		/* 对于序号是偶数的线段，除了方向取反，还需要处理高、低点 */
		current = start + 1;
		while (current < end - 1) // 这个条件是如何写出来的： 看下图，BC和CD，要求处理BC的时候，后面至少还有一个CD，才处理BC；因此如果，CD后面就是end，那么就有 BC < end - 1了
		{
			if ((*current).getDirection() == DESCENDING)
			{
/*
              B                       A
             /\                        \
            /  \                        \
           /    \         D              \      C
          /      \        /               \    /\
         /        \      /     ===>        \  /  \
        /          \    /                   \/    \
       A            \  /                    B      \
                     \/                             \
                     C                               D
*/
				(*current).Low = (*(current - 1)).getLow();
				(*current).High = (*(current + 1)).getHigh();
			}
			else
			{
/*
                    C                            D
                   /\                           /
                  /  \                         /
        A        /    \                  B    /
         \      /      \      ===>       /\  /
          \    /        \               /  \/
           \  /         D              /    C
            \/                        /
            B                        A

*/
				assert((*current).getDirection() == ASCENDING);
				(*current).High = (*(current - 1)).getHigh();
				(*current).Low = (*(current + 1)).getLow();
			}
			(*current).d = -(*current).getDirection();

			current += 2;
		}
	}

	static void NormalizeV2(baseItemIterator start, baseItemIterator end)
	{
		/* 这个函数，处理各个相邻线段，连接点处，最低值、最高值 不相等的情况；出现这种情况的原因，是因为，线段的最低值、最高值出现在线段中间某根K线，而不是出现在连接点处 */
		baseItemIterator curr = start;

		while (curr != end - 1)
		{
			baseItemType &former = *curr;
			baseItemType &latter = *(curr + 1);


			if (former.getDirection() == ASCENDING)
			{
				latter.High = former.High = max(former.getHigh(), latter.getHigh());
			}
			else
			{
				latter.Low = former.Low = min(former.getLow(), latter.getLow());
			}
			curr++;
		}
	}

	static ContainerType* startFenXianDuan(baseItemIterator start, baseItemIterator end)
	{
		NormalizeV2(start, end);

		baseItemType_Container* backupBeforeNormalize = (baseItemType_Container*)NULL;
		ContainerType* resultSet = (ContainerType*)NULL;

		baseItemIterator biStart = start;
		baseItemIterator biFormer = start;
		Direction d = ENCLOSING;
		while (end - biFormer > 2)
		{
			d = IComparable::getDirection(*biFormer, *(biFormer + 2));
			Direction dXian = (*biFormer).getDirection();
			if (d == ENCLOSING || d == -dXian) // d == -dXian  是为了规避normalize
			{
				biFormer += 1;
				continue;
			}
			else
				break;
		}

		if (end - biFormer <= 2) return resultSet;

		if (d == -(*biFormer).getDirection())
		{
			assert(0); // normalize 应该不会再被调用了，由于前面加了 d == -dXian条件
			backupBeforeNormalize = new baseItemType_Container();
			backupBeforeNormalize->assign(start,end);

			Normalize(d, biFormer, end);
		}

		analyzeStack CharacVecStack;

		baseItemIterator biLatter = biFormer + 2;

#ifdef DEBUG
		// 这段代码是用来调试死循环的。 
		int debugCnt = 0;
		baseItemIterator oldBiFormer = biFormer;
		baseItemIterator oldBiLatter = biLatter;
		int count = 0;
#endif

		do 
		{
			/*在线段中，寻找可能出现转折的那一笔的位置； 包括，前包后、方向与原线段相反；不包括：后包前（因为有新高或新低）*/
			while  (biFormer < end - 2 &&  (IComparable::getDirection(*biFormer, *biLatter) == d || (*biFormer << *biLatter) || (*biFormer >> *biLatter)) )
			{
				CharacVecStack.push_back(CharacterVec(biFormer + 1, biLatter - 1));
				biFormer = biLatter;

				if (biLatter < end - 2)
				{
					biLatter += 2;
				}
				else
				{
					/* baseItem快到最后了 */
					if (!resultSet)
					{
						resultSet = new ContainerType();
					}
					resultSet->push_back(XianDuanClass(biStart, biLatter, d));
#ifdef DEBUG
					debugCnt++;
#endif
				}
			}

			if (biFormer >= end - 2)
			{
				break;
			}

			if (FXD_Case1(-d, biFormer + 1, end, CharacVecStack) == false)
			{
				/* 原线段延续*/
				biFormer = CharacVecStack.back().start - 1;
				biLatter = CharacVecStack.back().end + 1;
				
				
#ifdef DEBUG
				// 这段代码是用来调试死循环的。 
				if (((*biFormer) == (*oldBiFormer)) && ((*biLatter) == (*oldBiLatter)))
				{
					count++;
				}
				else
				{
					count = 0;
					oldBiFormer = biFormer;
					oldBiLatter = biLatter;
				}

				if (count == 4)
					printf("break me here\n");

				if (count == 5)
					break;
#endif

				if (biLatter >= end)  break;

				if (IComparable::getDirection(*biFormer, *biLatter) == d || (*biFormer >> *biLatter))
				{
					/*
                    情形1： biFormer 与 biLatter之间的方向， 与 原线段方向相同

                                                    /
                               /\                  /
                              /  \          /\  biLatter
                       /\ biFormer\        /  \  /
                      /  \  /      \      /    \/
                     /    \/        \    /
                    /                \  /        
                                      \/
                   |<  原线段 >|<-  特征向量 ->|<- 原线段延续 ->|

                   情形2： biFormer 包含 biLatter

                                 /\
                                /  \                  /\
                       /\      /    \        /\      /  \  /
                      /  \ biFormer  \      /  \ biLatter\/
                     /    \  /        \    /    \  /
                    /      \/          \  /      \/
                   /                    \/
                   
                   |<   原线段  >|<-  特征向量 ->|<- 新线段 ->|
                                     
            
					*/
					if (biLatter < end - 2)
					{
						CharacVecStack.pop_back();
						
#if 0						
						if (*biFormer >> *biLatter)
						{
							biFormer = biLatter;
							biLatter += 2;
						}
#endif
						continue;
					}
					else
					{
						/* baseItem快到最后了 */
						if (!resultSet)
						{
							resultSet = new ContainerType();
						}
						resultSet->push_back(XianDuanClass(biStart, biFormer, d));
#ifdef DEBUG
						debugCnt++;
#endif
						break;
					}

				} else
				{
					/*
                    情形3： biFormer 与 biLatter之间的方向， 与 原线段方向相反

                               /\                          /
                              /  \                  /\    /
                       /\ biFormer\        /\      /  \  /
                      /  \  /      \      /  \ biLatter\/
                     /    \/        \    /    \  /
                    /                \  /      \/
                                      \/
                   |<  原线段 >|<-  特征向量 ->|<- 新线段 ->|
                               |   形成新线段  |


                   情形4： biFormer 被 biLatter 包含

                                                      /\
                               /\                    /  \  /
                              /  \                  /    \/
                       /\ biFormer\        /\   biLatter
                      /  \  /      \      /  \    /
                     /    \/        \    /    \  /
                    /                \  /      \/
                                      \/
                   |<  原线段 >|<-  特征向量 ->|<- 原线段延续 ->|
                               |   形成新线段  |


					*/

					// 将特征向量 作为 一个新的线段
					if (!resultSet)
					{
						resultSet = new ContainerType();
					}
					// 原线段
					resultSet->push_back(XianDuanClass(biStart, biFormer, d));
					// 特征向量形成新线段
					resultSet->push_back(XianDuanClass(biFormer + 1, biLatter - 1, -d));
					// 新线段
					CharacVecStack.clear();
					biFormer = biStart = biLatter;
#if 0
					/* 新的线段，开始的部分，可能有一组互相包含的线段 */
					baseItemType temp = *biFormer;
					while (biFormer < end - 2)
					{
						if (temp >> *(biFormer + 2))
						{
							temp = *(biFormer + 2);
							biFormer += 2;
							continue;
						}
						else
							break;
					}
					if (biFormer < end - 2)
						biLatter = biFormer + 2;
#endif
					if (biLatter < end - 2)
						biLatter += 2;

				}

			} else
			{
				/* 原线段被破坏，将该线段添加到container中 */
				if (!resultSet)
				{
					resultSet = new ContainerType();
				}
				resultSet->push_back(XianDuanClass(biStart, biFormer, d));

				int debugEdgeCnt = biFormer - biStart + 1;

#ifdef DEBUG
				debugCnt++;
				if (debugCnt == 67)
					printf("break me here\n");
#endif

				/* 新线段需要 翻转方向 */
				d = -d;
				/* 新的线段的第一笔*/
				biStart = ++ biFormer;
				/* 新的线段的特征向量栈初始化 */
				CharacVecStack.clear();
#if 0
				/* 新的线段，开始的部分，有一组互相包含的线段，可以看成是之前线段的特征向量，可以应用包含关系。合并它们 */
				baseItemType temp = *biFormer;
				while (biFormer < end - 2)
				{
					if (temp >> *(biFormer + 2))
					{
						temp = *(biFormer + 2);
						biFormer += 2;
						continue;
					}
					else
						break;
				}
				biLatter = biFormer + 2;
#endif
				if (biFormer < end - 2)
					biLatter = biFormer + 2;
			}
		} while (1);

		if (backupBeforeNormalize)
		{
			XianDuanClass::baseItems->assign(backupBeforeNormalize->begin(), backupBeforeNormalize->end());
			delete backupBeforeNormalize;
		}

		return resultSet;
	}