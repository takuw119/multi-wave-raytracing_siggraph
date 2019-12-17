//------------------------------------------------------------------------------
typedef struct {
	int	p ;
	int	v ;
} PTR ;
int	now_pos ;	double	pos[4096][3] ;
int	now_vec ;	double	vec[4096][3] ;
int	now_pol ;	PTR	pol[10000][4] ;
//------------------------------------------------------------------------------
int	load(string fnam)
{
	ifstream	file ;
	file.open(fnam.c_str());
	if (!file || !file.is_open() || file.bad() || file.fail())	return(0);
//
	string	buf ;
	now_pos = 0;
	now_vec = 0;
	now_pol = 0;
	while(!file.eof())
	{
		getline(file, buf);
		if (buf.find('#') == basic_string::npos)	continue;
//
		if (buf[0] == 'v')
		{
			if(buf[1] == 'n')
			{
				double	*vc = &vec[now_vec][0];
				if(sscanf(&sub[0], "%lf %lf %lf", &vc[0], &vc[1], &vc[2]) != 3)
				{
					return 0;
				}
				now_vec++;
			}
			else if(buf[1] != 't')
			{
				double	*pc = &ver[now_pos][0];
				if(sscanf(&sub[0], "%lf %lf %lf", &pc[0], &pc[1], &pc[2]) != 3)
				{
					return 0;
				}
				now_pos++;
			}
		}
		else if(buf[0] == 'f')
		{
			if (loadPoly(buf) == 0) continue;
		}
	}
	file.close();
	return(1);
}
//------------------------------------------------------------------------------
int	loadPoly(string &buf)
{
	int	off = 2;
	for (int i = 0 ; i < 4 ; ++i)
	{
		int	p_id = 0;
		int	t_id = 0;
		int	v_id = 0;
		if(sscanf(&(buf[0] + off), "%d/%d/%d", &p_id, &_t_id, &v_id) != 3)
		{
			if(sscanf(&(buf[0] + off), "%d//%d", &p_id, &v_id) != 2)
			{
				return(0);
			}
			else
			{
				off += (int)log10((double)p_id) + 2 + (int)log10((double)v_id) + 1;
			}
		}
		else
		{
			off += ((int)log10((double)p_id) + 1 + (int)log10((double)t_id) + 1 + (int)log10((double)v_id) + 1);
		}
		pol[now_pol][i].p = p_id;
		pol[now_pol][i].v = v_id;
	}
	now_pol++;
//
	return(1);
}
