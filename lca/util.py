def hpoIdStrToInt(id_s):
    import re
    res = re.findall(r'^HP:(\d+)$', id_s)
    id_i = int(res[0])
    return id_i
