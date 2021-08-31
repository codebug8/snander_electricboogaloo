/*
 *
 */

#if 0

// crappy ecc status check code

if((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_GIGADEVICE) &&
	((flash_info->dev_id == _SPI_NAND_DEVICE_ID_GD5F1GQ4UAYIG) ||
	 (flash_info->dev_id == _SPI_NAND_DEVICE_ID_GD5F1GQ4UBYIG) ||
	 (flash_info->dev_id == _SPI_NAND_DEVICE_ID_GD5F1GQ4UCYIG) ||
	 (flash_info->dev_id == _SPI_NAND_DEVICE_ID_GD5F1GQ4UEYIS) ||
	 (flash_info->dev_id == _SPI_NAND_DEVICE_ID_GD5F2GQ4UBYIG) ||
	 (flash_info->dev_id == _SPI_NAND_DEVICE_ID_GD5F2GQ4UE9IS) ||
	 (flash_info->dev_id == _SPI_NAND_DEVICE_ID_GD5F2GQ4UCYIG) ||
	 (flash_info->dev_id == _SPI_NAND_DEVICE_ID_GD5F4GQ4UBYIG) ||
	 (flash_info->dev_id == _SPI_NAND_DEVICE_ID_GD5F4GQ4UCYIG)))
{
	if((flash_info->dev_id == _SPI_NAND_DEVICE_ID_GD5F1GQ4UAYIG) ||
		(flash_info->dev_id == _SPI_NAND_DEVICE_ID_GD5F1GQ4UBYIG) ||
		(flash_info->dev_id == _SPI_NAND_DEVICE_ID_GD5F1GQ4UEYIS) ||
		(flash_info->dev_id == _SPI_NAND_DEVICE_ID_GD5F2GQ4UBYIG) ||
		(flash_info->dev_id == _SPI_NAND_DEVICE_ID_GD5F2GQ4UE9IS) ||
		(flash_info->dev_id == _SPI_NAND_DEVICE_ID_GD5F4GQ4UBYIG))
	{
		if(((status & 0x30) >> 4) == 0x2 )
		{
			rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
		}
	}

	if((flash_info->dev_id == _SPI_NAND_DEVICE_ID_GD5F1GQ4UCYIG) ||
		(flash_info->dev_id == _SPI_NAND_DEVICE_ID_GD5F2GQ4UCYIG) ||
		(flash_info->dev_id == _SPI_NAND_DEVICE_ID_GD5F4GQ4UCYIG))
	{
		if(((status & 0x70) >> 4) == 0x7)
		{
			rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
		}
	}
}
else if(flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_MXIC)
{
	if(((status & 0x30) >> 4) == 0x2 )
	{
		rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
	}
}
else if((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_ESMT) &&
	((flash_info->dev_id == _SPI_NAND_DEVICE_ID_F50L512M41A) ||
	 (flash_info->dev_id == _SPI_NAND_DEVICE_ID_F50L1G41A0) ||
	 (flash_info->dev_id == _SPI_NAND_DEVICE_ID_F50L1G41LB) ||
	 (flash_info->dev_id == _SPI_NAND_DEVICE_ID_F50L2G41LB)))
{
	if(((status & 0x30) >> 4) == 0x2)
	{
		rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
	}
}
else if((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_ZENTEL) &&
	((flash_info->dev_id == _SPI_NAND_DEVICE_ID_A5U12A21ASC) ||
	 (flash_info->dev_id == _SPI_NAND_DEVICE_ID_A5U1GA21BWS)))
{
	if(((status & 0x30) >> 4) == 0x2 )
	{
		rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
	}
}
else if(flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_ETRON)
{
	if(((status & 0x30) >> 4) == 0x2)
	{
		rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
	}
}
else if(flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_TOSHIBA)
{
	if(((status & 0x30) >> 4) == 0x2)
	{
		rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
	}
}
else if(flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_MICRON)
{
	if(((status & 0x70) >> 4) == 0x2)
	{
		rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
	}
}
else if(flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_HEYANG)
{
	if(((status & 0x30) >> 4) == 0x2)
	{
		rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
	}
}
else if(((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_PN) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_PN26G01AWSIUG)) ||
	((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_PN) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_PN26G02AWSIUG)))
{
	if(((status & 0x30) >> 4) == 0x2)
	{
		rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
	}
}
else if(((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_ATO) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_ATO25D2GA)) ||
	((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_ATO_2) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_ATO25D2GB)))
{
	if(((status & 0x30) >> 4) == 0x2)
	{
		rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
	}
}
else if(((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_FM) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_FM25S01)) ||
	((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_FM) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_FM25S01A)))
{
	if(((status & 0x30) >> 4) == 0x2)
	{
		rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
	}
}
else if(((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_FM) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_FM25G01B)) ||
	((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_FM) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_FM25G02B)) ||
	((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_FM) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_FM25G02C)))
{
	if(((status & 0x70) >> 4) == 0x7)
	{
		rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
	}
}
else if(((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_XTX) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_XT26G02B)))
{
	if(((status & 0x70) >> 4) == 0x7)
	{
		rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
	}
}
else if (((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_XTX) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_XT26G01A)))
{
	if(((status & 0x3C) >> 2) == 0x8)
	{
		rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
	}
}
else if (((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_XTX) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_XT26G02A)))
{
	if(((status & 0x30) >> 4) == 0x2 )
	{
		rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
	}
}
else if(((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_MIRA) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_PSU1GS20BN)))
{
	if(((status & 0x30) >> 4) == 0x2 )
	{
		rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
	}
}
else if (((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_BIWIN) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_BWJX08U)) ||
	((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_BIWIN) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_BWET08U)))
{
	if(((status & 0x30) >> 4) == 0x2)
	{
		rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
	}
}
else if (((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_FORESEE) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_FS35ND02GS2F1)) ||
	((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_FORESEE) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_FS35ND02GD1F1)) ||
	((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_FORESEE) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_FS35ND01GS1F1)) ||
	((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_FORESEE) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_FS35ND01GD1F1)))
{
	if(((status & 0x70) >> 4) == 0x7)
	{
		rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
	}
}
else if(((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_DS) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_DS35Q2GA)) ||
	((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_DS) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_DS35Q1GA)))
{
	if(((status & 0x30) >> 4) == 0x2)
	{
		rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
	}
}
else if(((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_FISON) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_CS11G0T0A0AA)) ||
	((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_FISON) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_CS11G1T0A0AA)) ||
	((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_FISON) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_CS11G0G0A0AA)))
{
	if(((status & 0x70) >> 4) == 0x7)
	{
		rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
	}
}
else if(((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_TYM) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_TYM25D2GA01)) ||
	((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_TYM) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_TYM25D2GA02)) ||
	((flash_info->mfr_id == _SPI_NAND_MANUFACTURER_ID_TYM) && (flash_info->dev_id == _SPI_NAND_DEVICE_ID_TYM25D1GA03)))
{
	if(((status & 0x30) >> 4) == 0x2)
	{
		rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
	}
}

if(rtn_status == SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK)
{
	spi_nand_info("[spinand_ecc_fail_check] : ECC cannot recover detected !, page = 0x%x\n", page_number);
}
#endif

#if 0
// block unlocking code?
unsigned char feature;

_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1,"SPI NAND Chip Init : Unlock all block and Enable Quad Mode\n");

if(((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_GIGADEVICE) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_GD5F1GQ4UAYIG)) ||
	((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_GIGADEVICE) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_GD5F1GQ4UBYIG)) ||
	((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_GIGADEVICE) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_GD5F1GQ4UCYIG)) ||
	((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_GIGADEVICE) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_GD5F1GQ4UEYIS)) ||
	((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_GIGADEVICE) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_GD5F2GQ4UBYIG)) ||
	((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_GIGADEVICE) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_GD5F2GQ4UE9IS)) ||
	((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_GIGADEVICE) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_GD5F2GQ4UCYIG)) ||
	((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_GIGADEVICE) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_GD5F4GQ4UBYIG)) ||
	((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_GIGADEVICE) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_GD5F4GQ4UCYIG)))
{
	/* 1. Unlock All block */
	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	feature &= 0xC1;
	spi_nand_protocol_set_status_reg_1(spi_controller, feature);

	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "After Unlock all block setup, the status register1 = 0x%x\n", feature);

	/* 2. Enable Qual mode */
	spi_nand_protocol_get_status_reg_2(spi_controller,&feature);
	feature |= 0x1;
	spi_nand_protocol_set_status_reg_2(spi_controller,feature);

	spi_nand_protocol_get_status_reg_2(spi_controller,&feature);
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "After enable qual mode setup, the status register2 = 0x%x\n", feature);
}
else if((ptr_device_t->mfr_id) == _SPI_NAND_MANUFACTURER_ID_MXIC)
{
	/* 1. Unlock All block */
	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	feature &= 0xC1;
	spi_nand_protocol_set_status_reg_1(spi_controller, feature);

	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "After Unlock all block setup, the status register1 = 0x%x\n", feature);

	/* 2. Enable Qual mode */
	spi_nand_protocol_get_status_reg_2(spi_controller,&feature);
	feature |= 0x1;
	spi_nand_protocol_set_status_reg_2(spi_controller,feature);

	spi_nand_protocol_get_status_reg_2(spi_controller,&feature);
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "After enable qual mode setup, the status register2 = 0x%x\n", feature);
}
else if( (ptr_device_t->mfr_id) == _SPI_NAND_MANUFACTURER_ID_WINBOND)
{
	if(((ptr_device_t->feature) & SPI_NAND_FLASH_DIE_SELECT_1_HAVE)) {
		_die_id = 0x00;
		spi_nand_protocol_die_select_1(spi_controller, _die_id);
	}

	/* Enable to modify the status regsiter 1 */
	feature = 0x58;
	spi_nand_protocol_set_status_reg_2(spi_controller,feature);

	/* Unlock all block and Enable Qual mode */
	feature = 0x81;
	spi_nand_protocol_set_status_reg_1(spi_controller, feature);

	/* Disable to modify the status regsiter 1 */
	feature = 0x18;
	spi_nand_protocol_set_status_reg_2(spi_controller,feature);

	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "After Unlock all block setup, the status register1 = 0x%x\n", feature);

	/* Unlock all block for Die_1 */
	if( ((ptr_device_t->feature) & SPI_NAND_FLASH_DIE_SELECT_1_HAVE) )
	{
		_die_id = 0x01;
		spi_nand_protocol_die_select_1(spi_controller, _die_id);

		/* Enable to modify the status regsiter 1 */
		feature = 0x58;
		spi_nand_protocol_set_status_reg_2(spi_controller,feature);

		/* Unlock all block and Enable Qual mode */
		feature = 0x81;
		spi_nand_protocol_set_status_reg_1(spi_controller, feature);

		/* Disable to modify the status regsiter 1 */
		feature = 0x18;
		spi_nand_protocol_set_status_reg_2(spi_controller,feature);

		spi_nand_protocol_get_status_reg_1(spi_controller, &feature);

		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "After Unlock all block setup, the die %d status register1 = 0x%x\n", _die_id, feature);
	}
}
else if(((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_ESMT) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_F50L512M41A)) ||
		((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_ESMT) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_F50L1G41A0)))
{
	/* 1. Unlock All block */
	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	feature &= 0xC7;
	spi_nand_protocol_set_status_reg_1(spi_controller, feature);

	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "After Unlock all block setup, the status register1 = 0x%x\n", feature);
}
else if(((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_ESMT) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_F50L1G41LB)) ||
		((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_ESMT) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_F50L2G41LB)))
{
	if(((ptr_device_t->feature) & SPI_NAND_FLASH_DIE_SELECT_1_HAVE))
	{
		_die_id = 0x00;
		spi_nand_protocol_die_select_1(spi_controller, _die_id);
	}

	/* 1. Unlock All block */
	feature = 0x83;
	spi_nand_protocol_set_status_reg_1(spi_controller, feature);

	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "After Unlock all block setup, the status register1 = 0x%x\n", feature);

	/* Unlock all block for Die_1 */
	if(((ptr_device_t->feature) & SPI_NAND_FLASH_DIE_SELECT_1_HAVE))
	{
		_die_id = 0x01;
		spi_nand_protocol_die_select_1(spi_controller, _die_id);

		/* 1. Unlock All block */
		feature = 0x83;
		spi_nand_protocol_set_status_reg_1(spi_controller, feature);

		spi_nand_protocol_get_status_reg_1(spi_controller, &feature);

		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "After Unlock all block setup, the die %d status register1 = 0x%x\n", _die_id, feature);
	}
}
else if(((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_ZENTEL) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_A5U12A21ASC)) ||
		((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_ZENTEL) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_A5U1GA21BWS)))
{
	/* 1. Unlock All block */
	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	feature &= 0xC7;
	spi_nand_protocol_set_status_reg_1(spi_controller, feature);

	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "After Unlock all block setup, the status register1 = 0x%x\n", feature);
}
else if( (ptr_device_t->mfr_id) == _SPI_NAND_MANUFACTURER_ID_ETRON)
{
	/* 1. Unlock All block */
	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	feature &= 0xC1;
	spi_nand_protocol_set_status_reg_1(spi_controller, feature);

	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "After Unlock all block setup, the status register1 = 0x%x\n", feature);

	/* 2. Enable Qual mode */
	spi_nand_protocol_get_status_reg_2(spi_controller,&feature);
	feature |= 0x1;
	spi_nand_protocol_set_status_reg_2(spi_controller,feature);

	spi_nand_protocol_get_status_reg_2(spi_controller,&feature);
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "After enable qual mode setup, the status register2 = 0x%x\n", feature);
}
else if( (ptr_device_t->mfr_id) == _SPI_NAND_MANUFACTURER_ID_TOSHIBA)
{
	/* 1. Unlock All block */
	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	feature &= 0xC7;
	spi_nand_protocol_set_status_reg_1(spi_controller, feature);

	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1,"After Unlock all block setup, the status register1 = 0x%x\n", feature);
}

else if( (ptr_device_t->mfr_id) == _SPI_NAND_MANUFACTURER_ID_MICRON)
{
	if(((ptr_device_t->feature) & SPI_NAND_FLASH_DIE_SELECT_2_HAVE)) {
		_die_id = 0x00;
		spi_nand_protocol_die_select_2(spi_controller, _die_id);
	}

	/* 1. Unlock All block */
	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	feature &= 0x83;
	spi_nand_protocol_set_status_reg_1(spi_controller, feature);

	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1,"After Unlock all block setup, the status register1 = 0x%x\n", feature);

	/* Unlock all block for Die_1 */
	if( ((ptr_device_t->feature) & SPI_NAND_FLASH_DIE_SELECT_2_HAVE) )
	{
		_die_id = 0x01;
		spi_nand_protocol_die_select_2(spi_controller, _die_id);

		/* 1. Unlock All block */
		spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
		feature &= 0x83;
		spi_nand_protocol_set_status_reg_1(spi_controller, feature);

		spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1,"After Unlock all block setup, the die %d status register1 = 0x%x\n", _die_id, feature);
	}
}
else if( (ptr_device_t->mfr_id) == _SPI_NAND_MANUFACTURER_ID_HEYANG)
{
	/* 1. Unlock All block */
	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	feature &= 0xC7;
	spi_nand_protocol_set_status_reg_1(spi_controller, feature);

	/* 2. Enable Qual mode */
	spi_nand_protocol_get_status_reg_2(spi_controller,&feature);
	feature |= 0x1;
	spi_nand_protocol_set_status_reg_2(spi_controller,feature);
}
else if(((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_PN) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_PN26G01AWSIUG)) ||
		((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_PN) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_PN26G02AWSIUG)))
{
	/* 1. Unlock All block */
	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	feature &= 0xC7;
	spi_nand_protocol_set_status_reg_1(spi_controller, feature);

	/* 2. Enable Qual mode */
	spi_nand_protocol_get_status_reg_2(spi_controller,&feature);
	feature |= 0x1;
	spi_nand_protocol_set_status_reg_2(spi_controller,feature);
}
else if( ((ptr_device_t->mfr_id) == _SPI_NAND_MANUFACTURER_ID_ATO) ||
	 ((ptr_device_t->mfr_id) == _SPI_NAND_MANUFACTURER_ID_ATO_2) )
{
	/* 1. Unlock All block */
	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	feature &= 0xC7;
	spi_nand_protocol_set_status_reg_1(spi_controller, feature);
}
else if(((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_FM) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_FM25S01)) ||
		((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_FM) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_FM25S01A)))
{
	/* 1. Unlock All block */
	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	feature &= 0x87;
	spi_nand_protocol_set_status_reg_1(spi_controller, feature);
}
else if(((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_FM) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_FM25G01B)) ||
		((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_FM) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_FM25G02B)) ||
		((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_FM) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_FM25G02C)))
{
	/* 1. Unlock All block */
	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	feature &= 0xC7;
	spi_nand_protocol_set_status_reg_1(spi_controller, feature);

	/* 2. Enable Qual mode */
	spi_nand_protocol_get_status_reg_2(spi_controller,&feature);
	feature |= 0x1;
	spi_nand_protocol_set_status_reg_2(spi_controller,feature);
}
else if(((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_XTX) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_XT26G02B)) ||
		((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_XTX) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_XT26G02A)))
{
	/* 1. Unlock All block */
	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	feature &= 0xC7;
	spi_nand_protocol_set_status_reg_1(spi_controller, feature);

	/* 2. Enable Qual mode */
	spi_nand_protocol_get_status_reg_2(spi_controller,&feature);
	feature |= 0x1;
	spi_nand_protocol_set_status_reg_2(spi_controller,feature);
}
else if(((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_MIRA) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_PSU1GS20BN)))
{
	/* 1. Unlock All block */
	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	feature &= 0xC7;
	spi_nand_protocol_set_status_reg_1(spi_controller, feature);

	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "After Unlock all block setup, the status register1 = 0x%x\n", feature);
}
else if(((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_BIWIN) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_BWJX08U)) ||
		((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_BIWIN) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_BWET08U)))
{
	/* 1. Unlock All block */
	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	feature &= 0xC7;
	spi_nand_protocol_set_status_reg_1(spi_controller, feature);

	/* 2. Enable Qual mode */
	spi_nand_protocol_get_status_reg_2(spi_controller,&feature);
	feature |= 0x1;
	spi_nand_protocol_set_status_reg_2(spi_controller,feature);
}
else if(((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_FORESEE) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_FS35ND02GS2F1)) ||
		((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_FORESEE) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_FS35ND02GD1F1)) ||
		((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_FORESEE) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_FS35ND01GS1F1)) ||
		((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_FORESEE) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_FS35ND01GD1F1)))
{
	/* 1. Unlock All block */
	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	feature &= 0xC7;
	spi_nand_protocol_set_status_reg_1(spi_controller, feature);

	/* 2. Enable Qual mode */
	spi_nand_protocol_get_status_reg_2(spi_controller,&feature);
	feature |= 0x1;
	spi_nand_protocol_set_status_reg_2(spi_controller,feature);
}
else if(((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_DS) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_DS35Q2GA)) ||
	((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_DS) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_DS35Q1GA)))
{
	/* 1. Unlock All block */
	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	feature &= 0xC7;
	spi_nand_protocol_set_status_reg_1(spi_controller, feature);

	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "After Unlock all block setup, the status register1 = 0x%x\n", feature);
}
else if(((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_FISON) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_CS11G0T0A0AA)) ||
	((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_FISON) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_CS11G1T0A0AA)) ||
	((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_FISON) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_CS11G0G0A0AA)))
{
	/* 1. Unlock All block */
	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	feature &= 0xC7;
	spi_nand_protocol_set_status_reg_1(spi_controller, feature);

	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "After Unlock all block setup, the status register1 = 0x%x\n", feature);
}
else if(((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_TYM) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_TYM25D2GA01)) ||
	((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_TYM) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_TYM25D2GA02)) ||
	((ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_TYM) && (ptr_device_t->dev_id == _SPI_NAND_DEVICE_ID_TYM25D1GA03)))
{
	/* 1. Unlock All block */
	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	feature &= 0xC7;
	spi_nand_protocol_set_status_reg_1(spi_controller, feature);

	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "After Unlock all block setup, the status register1 = 0x%x\n", feature);
}
else
{
	/* 1. Unlock All block */
	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	feature &= 0xC1;
	spi_nand_protocol_set_status_reg_1(spi_controller, feature);

	spi_nand_protocol_get_status_reg_1(spi_controller, &feature);
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "After Unlock all block setup, the status register1 = 0x%x\n", feature);

	/* 2. Enable Qual mode */
	spi_nand_protocol_get_status_reg_2(spi_controller, &feature);
	feature |= 0x1;
	spi_nand_protocol_set_status_reg_2(spi_controller, feature);

	spi_nand_protocol_get_status_reg_2(spi_controller, &feature);
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "After enable qual mode setup, the status register2 = 0x%x\n", feature);
}
#endif
