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

	// ����ָ�������ɶ��ٷ�Ӱ��
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

	// spaceX��spaceY̫��ԭʼͼ������ָ�
	if(nRows ==1 && nCols == 1)
	{
		return FE_UNKNOWN_ERROR; // ԭʼͼ����Ҫ�ָ�
	}

	GDALDriver *poDriver = poDataset->GetDriver();
	int nBandCount   = poDataset->GetRasterCount();
	GDALDataType eDataType = poDataset->GetRasterBand(1)->GetRasterDataType();
	int nBytesPerSample = GDALGetDataTypeSize(eDataType)/8;

	QString strFilenameDest;
	int pos = filename.lastIndexOf(tr("."));
	QString strExt = filename.right(filename.size()-pos);

	// ͬʱ����һ���ı��ļ��洢һ���ж��ٸ��ָ�Ӱ��
	QString strSplitMetaFile = parafile;
	
	// д��ͼ������
	void *pDataSrc  = CPLMalloc(nBytesPerSample * spaceX * 1);

	// ��������һ�����ݲ���spaceX/spaceY�������Ҫ����ʵ�ʶ�ȡ��ͼ���ȡ��߶�
	// һ����spaceX,����spaceY
	int nRealReadX = 0;
	int nRealReadY = 0;

	// ��ʼ��ȡԭʼͼ���λ��
	int iStartY = 0;
	int iStartX = 0;

	vector<QString> vecStringFilename;
	vector<TFWPara> vecTFW; //Ϊ��ƴ�ӷ��㣬ʹ�����TFW�ļ����һ��

	// ��ʼ����ָ�ͼ��
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

			// ����ͼ������
			QString strSplitExt;
			strSplitExt= tr("__r%1_c%2").arg(iR).arg(iC);
			strFilenameDest = filename.left(pos)+strSplitExt+strExt;			

			// ����ʵ����Ҫ��ȡ��ͼ������߶�
			if(iR == nRows )
				nRealReadY = nImageHeight-((iR-1)*(spaceY-overlapY));
			else
				nRealReadY = spaceY;
			if(iC == nCols)			
				nRealReadX = nImageWidth-((iC-1)*(spaceX-overlapX));			
			else
				nRealReadX = spaceX;

			// ������ͼ��
			GDALDataset *poDatasetDest = poDriver->Create( strFilenameDest.toLocal8Bit().data(),
				nRealReadX, nRealReadY, nBandCount,
				eDataType, poDriver->GetMetadata());	

			// ͬʱ����Tiff�ļ�
			if(iR == 1)
				iStartY = 0;
			else
				iStartY = (iR-1)*(spaceY-overlapY);
			if(iC == 1)
				iStartX = 0;
			else
				iStartX = (iC-1)*(spaceX-overlapX);		

			// �����ȡԭʼͼ�����ʼλ��
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
			// ���ͳһת����,����ʱ��һ��
			vecStringFilename.push_back(strFilenameDest);

			TFWPara tfw;
			// ����TFW�ļ�
			tfw.lfDX = 1;
			// ��δ����Ͻǿ�ʼΪ-1
			tfw.lfDY = -1;
			tfw.lfU1 = 0;
			tfw.lfU2 = 0;

			// ���Ͻ���������
			tfw.lfXUL = iStartX;
			tfw.lfYUL = -iStartY;

			vecTFW.push_back(tfw);
		} // for iC
	} // for iR

	CPLFree(pDataSrc);


	// ���ɲ����ļ� ,����Ƿָ�����ļ�
	FILE *streamin = fopen(strSplitMetaFile.toLocal8Bit().data(), "w");
	if(streamin == NULL)
	{
		return FE_UNKNOWN_ERROR;
	}
	fprintf(streamin, "%s\n", filename.toLocal8Bit().data());
	fprintf(streamin, "%d\n", (int)vecTFW.size());
	fprintf(streamin, "%d %d %d %d\n", spaceX, spaceY, overlapX, overlapY);

	// generate tfw��������global�п������ֿ�Ľ��
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