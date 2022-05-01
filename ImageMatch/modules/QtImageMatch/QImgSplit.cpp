#include "QImgSplit.h"


QImgSplit::QImgSplit(void)
{
}


QImgSplit::~QImgSplit(void)
{
}


void QImgSplit::setParameter( void* processParameters )
{
	QParameterImgSplit* pPara = (QParameterImgSplit*) processParameters;
	setParameterInternal(*pPara);
}

void QImgSplit::beginProcess()
{
	_isCanceled = false;
	
	SplitImage(_srcImg.toLocal8Bit().data(), _paraFile.toLocal8Bit().data(),
		_nSpaceX, _nSpaceY, _nOverlapX, _nOverlapY, _pFunc, _pProgressData);
}

void QImgSplit::setParameterInternal( const QParameterImgSplit& para )
{
	if(&para != NULL)
	{
		_srcImg  = para.srcImg;
		_paraFile = para.paraFile;
		_nSpaceX  = para.nSpaceX;
		_nSpaceY  = para.nSpaceY;
		_nOverlapX  = para.nOverlapX;
		_nOverlapY  = para.nOverlapY;
		_pFunc = para.pFunc;
		_pProgressData = para.pProgressData;
	}
}


FuncErr QImgSplit::SplitImage(QString filename, QString parafile, int spaceX, int spaceY, int overlapX, int overlapY,
	GDALProgressFunc pFunProgress, void *pProgressData )
{
	if(spaceX<0 || spaceY<0 || overlapX<0 || overlapY<0)
	{
		return FE_IllegalArg;
	}

	if(filename == tr("") || parafile == tr(""))
	{
		return FE_IllegalArg;
	}

	GDALDataset *poDataset = (GDALDataset *) (GDALOpen(filename.toLocal8Bit().data(), GA_ReadOnly));
	if(poDataset == NULL)
	{
		return FE_OpenFailed;
	}

	int nImageWidth = poDataset->GetRasterXSize();
	int nImageHeight = poDataset->GetRasterYSize();

	if(spaceX > nImageWidth || spaceY > nImageHeight)
		return FE_IllegalArg;

	if(spaceX >= nImageWidth && spaceY >= nImageHeight)
		return FE_IllegalArg;

	// 计算分割方案可生成多少幅影像
	int nRows = 1;

	int nTempY = spaceY;
	while(nTempY<nImageHeight)
	{
		nRows++;
		nTempY += spaceY-overlapY;
	}

	int nCols = 1;
	int nTempX = spaceX;
	while(nTempX<nImageWidth)
	{
		nCols++;
		nTempX += spaceX-overlapX;
	}

	// spaceX与spaceY太大，原始图像无需分割
	if(nRows ==1 && nCols == 1)
	{
		return FE_UNKNOWN_ERROR; // 原始图像不需要分割
	}

	GDALDriver *poDriver = poDataset->GetDriver();
	int nBandCount   = poDataset->GetRasterCount();
	GDALDataType eDataType = poDataset->GetRasterBand(1)->GetRasterDataType();
	int nBytesPerSample = GDALGetDataTypeSize(eDataType)/8;

	QString strFilenameDest;
	int pos = filename.lastIndexOf(tr("."));
	QString strExt = filename.right(filename.size()-pos);

	// 同时生成一个文本文件存储一共有多少个分割影像
	QString strSplitMetaFile = parafile;
	
	// 写入图像数据
	void *pDataSrc  = CPLMalloc(nBytesPerSample * spaceX * 1);

	// 可能最后的一块数据不能spaceX/spaceY，因此需要计算实际读取的图像宽度、高度
	// 一般是spaceX,或者spaceY
	int nRealReadX = 0;
	int nRealReadY = 0;

	// 开始读取原始图像的位置
	int iStartY = 0;
	int iStartX = 0;

	vector<QString> vecStringFilename;
	vector<TFWPara> vecTFW; //为了拼接方便，使用这个TFW文件会好一点

	// 开始逐个分割图像
	for(int iR = 1; iR<=nRows; iR++)
	{
		for(int iC=1; iC<=nCols; iC++)
		{
			int nCurCell = (iR-1)*nCols+(iC-1);

			if(pFunProgress != NULL)				
			{
					if(pFunProgress((nCurCell*1.0)/(nRows*nCols), "", pProgressData) == 0)
					{
						CPLFree(pDataSrc);
						return FE_UserInterupt;
					}
			}

			// 计算图像名称
			QString strSplitExt;
			strSplitExt= tr("__r%1_c%2").arg(iR).arg(iC);
			strFilenameDest = filename.left(pos)+strSplitExt+strExt;			

			// 计算实际需要读取的图像宽度与高度
			if(iR == nRows )
				nRealReadY = nImageHeight-((iR-1)*(spaceY-overlapY));
			else
				nRealReadY = spaceY;
			if(iC == nCols)			
				nRealReadX = nImageWidth-((iC-1)*(spaceX-overlapX));			
			else
				nRealReadX = spaceX;

			// 创建新图像
			GDALDataset *poDatasetDest = poDriver->Create( strFilenameDest.toLocal8Bit().data(),
				nRealReadX, nRealReadY, nBandCount,
				eDataType, poDriver->GetMetadata());	

			// 同时生成Tiff文件
			if(iR == 1)
				iStartY = 0;
			else
				iStartY = (iR-1)*(spaceY-overlapY);
			if(iC == 1)
				iStartX = 0;
			else
				iStartX = (iC-1)*(spaceX-overlapX);		

			// 计算读取原始图像的起始位置
			// processing by line 
			for(int iBand=0; iBand<nBandCount; iBand++)
			{
				GDALRasterBand *poRasterBandSrc = poDataset->GetRasterBand(iBand+1);
				GDALRasterBand *poRasterBandDest = poDatasetDest->GetRasterBand(iBand+1);

				for(int iY=0; iY<nRealReadY; iY++)
				{
					CPLErr eErr = poRasterBandSrc->RasterIO(GF_Read, iStartX, iY+iStartY,
						nRealReadX, 1, pDataSrc, nRealReadX, 1, eDataType, 0, 0);

					if(eErr != CE_None)
					{
						CPLFree(pDataSrc);
						return FE_UNKNOWN_ERROR;
					}			

					eErr =  poRasterBandDest->RasterIO(GF_Write, 0, iY, 
						nRealReadX, 1, pDataSrc, nRealReadX, 1, eDataType, 0, 0);

					if(eErr != CE_None)
					{
						CPLFree(pDataSrc);
						return FE_UNKNOWN_ERROR;
					}
				} // for iY
			} // for iBand


			GDALClose((GDALDatasetH) poDatasetDest) ;	
			// 最后统一转换吧,这样时间一致
			vecStringFilename.push_back(strFilenameDest);

			TFWPara tfw;
			// 生成TFW文件
			tfw.lfDX = 1;
			// 如何从左上角开始为-1
			tfw.lfDY = -1;
			tfw.lfU1 = 0;
			tfw.lfU2 = 0;

			// 左上角像素坐标
			tfw.lfXUL = iStartX;
			tfw.lfYUL = -iStartY;

			vecTFW.push_back(tfw);
		} // for iC
	} // for iR

	CPLFree(pDataSrc);


	// 生成参数文件 ,这个是分割参数文件
	FILE *streamin = fopen(strSplitMetaFile.toLocal8Bit().data(), "w");
	if(streamin == NULL)
	{
		return FE_UNKNOWN_ERROR;
	}
	fprintf(streamin, "%s\n", filename.toLocal8Bit().data());
	fprintf(streamin, "%d\n", (int)vecTFW.size());
	fprintf(streamin, "%d %d %d %d\n", spaceX, spaceY, overlapX, overlapY);

	// generate tfw，便于在global中看各个分块的结果
	for(int index=0; index<(int)vecTFW.size(); index++)
	{
		QString strFile = vecStringFilename[index];
		TFWPara tfw = vecTFW[index];
		QImgUtil::GenerateTFWFile(strFile, tfw);
		fprintf(streamin, "%0s\n", strFile.toLocal8Bit().data());
	}

	fclose(streamin);	
	return FE_None;
}