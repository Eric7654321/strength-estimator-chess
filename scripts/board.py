assert_flag = False
square = {'a1':0, 'b1':1, 'c1':2, 'd1':3, 'e1':4, 'f1':5, 'g1':6, 'h1':7,
          'a2':8, 'b2':9, 'c2':10, 'd2':11, 'e2':12, 'f2':13, 'g2':14, 'h2':15,
          'a3':16, 'b3':17, 'c3':18, 'd3':19, 'e3':20, 'f3':21, 'g3':22, 'h3':23,
          'a4':24, 'b4':25, 'c4':26, 'd4':27, 'e4':28, 'f4':29, 'g4':30, 'h4':31,
          'a5':32, 'b5':33, 'c5':34, 'd5':35, 'e5':36, 'f5':37, 'g5':38, 'h5':39,
          'a6':40, 'b6':41, 'c6':42, 'd6':43, 'e6':44, 'f6':45, 'g6':46, 'h6':47,
          'a7':48, 'b7':49, 'c7':50, 'd7':51, 'e7':52, 'f7':53, 'g7':54, 'h7':55,
          'a8':56, 'b8':57, 'c8':58, 'd8':59, 'e8':60, 'f8':61, 'g8':62, 'h8':63}

squareIDtoSTR = {value: key for key, value in square.items()}

piece = {'K':'\u265A', 'Q':'\u265B', 'R':'\u265C', 'B':'\u265D', 'N':'\u265E', 'P':'\u265F',
         'k':'\u2654', 'q':'\u2655', 'r':'\u2656', 'b':'\u2657', 'n':'\u2658', 'p':'\u2659',
         '-':'-'}

board = [i for i in 'RNBQKBNRPPPPPPPP--------------------------------pppppppprnbqkbnr']
king_pos = [4, 60]
pinned = [False for i in range(64)]

# (Δx, Δy)
direction = [(0, 1), (1, 1), (1, 0), (1, -1), (0, -1), (-1, -1), (-1, 0), (-1, 1)]
knight_direction = [(1, 2), (2, 1), (2, -1), (1, -2), (-1, -2), (-2, -1), (-2, 1), (-1, 2)]

# action_id = square_id(0-63) * 73 + move_id(0-72), (Δx, Δy)
move_id = {(0, 1):0,  (1, 1):1,  (1, 0):2,   (1, -1):3,  (0, -1):4,   (-1, -1):5,  (-1, 0):6,  (-1, 1):7,
           (0, 2):8,  (2, 2):9,  (2, 0):10,  (2, -2):11, (0, -2):12,  (-2, -2):13, (-2, 0):14, (-2, 2):15,
           (0, 3):16, (3, 3):17, (3, 0):18,  (3, -3):19, (0, -3):20,  (-3, -3):21, (-3, 0):22, (-3, 3):23,
           (0, 4):24, (4, 4):25, (4, 0):26,  (4, -4):27, (0, -4):28,  (-4, -4):29, (-4, 0):30, (-4, 4):31,
           (0, 5):32, (5, 5):33, (5, 0):34,  (5, -5):35, (0, -5):36,  (-5, -5):37, (-5, 0):38, (-5, 5):39,
           (0, 6):40, (6, 6):41, (6, 0):42,  (6, -6):43, (0, -6):44,  (-6, -6):45, (-6, 0):46, (-6, 6):47,
           (0, 7):48, (7, 7):49, (7, 0):50,  (7, -7):51, (0, -7):52,  (-7, -7):53, (-7, 0):54, (-7, 7):55,
           (1, 2):56, (2, 1):57, (2, -1):58, (1, -2):59, (-1, -2):60, (-2, -1):61, (-2, 1):62, (-1, 2):63}

promote_id = {(-1, 'R'):64, (-1, 'B'):65, (-1, 'N'):66, 
               (0, 'R'):67,  (0, 'B'):68,  (0, 'N'):69, 
               (1, 'R'):70,  (1, 'B'):71,  (1, 'N'):72}

def square2RC(square): #str -> 0-7
    return (int(square[1]) - 1, ord(square[0]) - ord('a')) 

def squareID2RC(square):
    return square // 8, square % 8

def removeCheckTakeMate(move, check=True, take=True):
    if check:
        newmove = move.replace('+', '')
        newmove = newmove.replace('#', '')
    if take:
        newmove = newmove.replace('x', '')
    return newmove

def outOfBoard(row, col):
    return row >= 8 or row < 0 or col >= 8 or col < 0

def colrow2Square(col, row): #0-7
    return square[chr(ord('a') + col) + str(row + 1)]

def showBoard():
    for i in range(7, -1, -1):
        output = str(i + 1)
        for b in board[i*8:(i+1)*8]:
            # output = output + '  ' + piece[b]
            output = output + '  ' + b
        print(output)
    print('   a  b  c  d  e  f  g  h')
    print()

def resetBoard(): # uppercase: white, lowercase: black
    global board, pinned, king_pos
    board = [i for i in 'RNBQKBNRPPPPPPPP--------------------------------pppppppprnbqkbnr']
    pinned = [False for i in range(64)]
    king_pos = [4, 60]

def findPinnedPieces(turn, debug=False):
    global pinned
    pinned = [False for i in range(64)]
    if turn == 'W':
        king_row, king_col = squareID2RC(king_pos[0])
        opp_b, opp_r, opp_q = 'b', 'r', 'q'
    else:
        king_row, king_col = squareID2RC(king_pos[1])
        opp_b, opp_r, opp_q = 'B', 'R', 'Q'
    for b_dir in [direction[i] for i in range(1, 8, 2)]:
        potential_pin = -1
        attack_piece_found = False
        # if debug:
        #     print('dir (col, row):', b_dir)
        for j in range(1, 8):
            piece_row, piece_col = king_row + b_dir[1]*j, king_col + b_dir[0]*j
            if outOfBoard(piece_row, piece_col):
                break
            check_piece = board[colrow2Square(piece_col, piece_row)]
            # if debug:
            #     print(f'check: {check_piece}, potential pin: {potential_pin}, attack found: {attack_piece_found}')
            if turn == 'W' and check_piece.isupper() or turn == 'B' and check_piece.islower():
                if potential_pin != -1:
                    break
                elif not attack_piece_found:
                    potential_pin = colrow2Square(piece_col, piece_row)
            elif check_piece == opp_b or check_piece == opp_q:
                if potential_pin != -1:
                    pinned[potential_pin] = True
                break
            elif check_piece != '-':
                break
            
    for r_dir in [direction[i] for i in range(0, 8, 2)]:
        potential_pin = -1
        attack_piece_found = False
        # if debug:
        #     print('dir (col, row):', r_dir)
        for j in range(1, 8):
            piece_row, piece_col = king_row + r_dir[1]*j, king_col + r_dir[0]*j
            if outOfBoard(piece_row, piece_col):
                break
            check_piece = board[colrow2Square(piece_col, piece_row)]
            # if debug:
            #     print(f'check: {check_piece}, potential pin: {potential_pin}, attack found: {attack_piece_found}')
            if turn == 'W' and check_piece.isupper() or turn == 'B' and check_piece.islower():
                if potential_pin != -1:
                    break
                elif not attack_piece_found:
                    potential_pin = colrow2Square(piece_col, piece_row)
            elif check_piece == opp_r or check_piece == opp_q:
                if potential_pin != -1:
                    pinned[potential_pin] = True
                break
            elif check_piece != '-':
                break
    
def actLongCastle(turn): # 0-0-0
    if turn == 'w':
        board[:5] = list('--KR-')
        # print(f'0-0-0[{4 * 73 + 12}]')
        # showBoard()
        king_pos[0] = 2
        return 'e1a1'
        return 4 * 73 + 14
    else:
        board[56:61] = list('--kr-')
        # print(f'0-0-0[{60 * 73 + 12}]')
        # showBoard()
        king_pos[1] = 58
        return 'e8a8'
        return 60 * 73 + 14

def actShortCastle(turn): # 0-0
    if turn == 'w':
        board[4:8] = list('-RK-')
        # print(f'0-0[{4 * 73 + 10}]')
        # showBoard()
        king_pos[0] = 6
        return 'e1h1'
        return 4 * 73 + 10
    else:
        board[60:] = list('-rk-')
        # print(f'0-0[{60 * 73 + 10}]')
        # showBoard()
        king_pos[1] = 62
        return 'e8h8'
        return 60 * 73 + 10

def actPawn(pawn, move):
    newmove = removeCheckTakeMate(move, check=True, take=False)
    from_col = newmove[0]
    if pawn == 'P':
        del_row = 1
        enpassant_row = 4
        moveto_row = '4'
        forward_id = 0
    else:
        del_row = -1
        enpassant_row = 3
        moveto_row = '5'
        forward_id = 4
    if '=' not in newmove:
        move_to = square[newmove[-2:]]
        move_from = square[from_col + str(int(newmove[-1])-del_row)]
        if 'x' not in newmove:
            action_id = forward_id
            if newmove[-1] == moveto_row and board[square[from_col + str(int(newmove[-1])-del_row)]] == '-':
                move_from = square[from_col + str(int(newmove[-1])-2*del_row)]
                action_id = forward_id + 8
        else:
            # print((ord(newmove[-2]) - ord(from_col), del_row))
            action_id = move_id[(ord(newmove[-2]) - ord(from_col), del_row)]
    else:
        move_to = square[newmove[-4:-2]]
        move_from = square[from_col + str(int(newmove[-3])-del_row)]
    # print(f'{move}, from: {move_from}, to: {move_to}')
    board[move_from] = '-'
    if move_from // 8 == enpassant_row and 'x' in move and board[move_to] == '-':
        # print('enpassant')
        board[colrow2Square(move_to % 8, move_from // 8)] = '-'
    board[move_to] = pawn
    ret_str = squareIDtoSTR[move_from] + squareIDtoSTR[move_to]
    if '=' in newmove:
        if pawn == 'P':
            board[move_to] = newmove[-1]
        else:
            board[move_to] = newmove[-1].lower()
        # if 'N' not in newmove:
        ret_str += newmove[-1].lower()
        # -1/rbn: 64-66, 0/rbn: 67-69, 1/rbn: 70-72
        # if 'Q' not in newmove:
        #     action_id = promote_id[(move_to % 8 - move_from % 8, newmove[-1])]
        # else:
        #     action_id = move_id[(move_to % 8 - move_from % 8, move_to // 8 - move_from // 8)]
    # print(f'{move}[{move_from * 73 + action_id}]')
    # showBoard()
    return ret_str
    return move_from * 73 + action_id

def actKnight(knight, move):
    newmove = removeCheckTakeMate(move)
    move_to = square[newmove[-2:]]
    move_from = -1
    to_row, to_col = square2RC(newmove[-2:])
    if len(newmove) == 3:
        for dir in knight_direction:
            from_col, from_row = to_col + dir[0], to_row + dir[1]
            if outOfBoard(from_col, from_row):
                continue
            else:
                if board[colrow2Square(from_col, from_row)] == knight:
                    if pinned[colrow2Square(from_col, from_row)]:
                        continue
                    move_from = colrow2Square(from_col, from_row)
                    action_id = move_id[(to_col - from_col, to_row - from_row)]
                    break
    elif len(newmove) == 4:
        from_line = newmove[1]
        for dir in knight_direction:
            from_col, from_row = to_col + dir[0], to_row + dir[1]
            if outOfBoard(from_col, from_row):
                continue
            else:
                if from_line.isdigit():
                    if from_row != ord(from_line) - ord('1'):
                        continue
                elif from_line.isalpha():
                    if from_col != ord(from_line) - ord('a'):
                        continue
                if board[colrow2Square(from_col, from_row)] == knight:
                    if pinned[colrow2Square(from_col, from_row)]:
                        continue
                    move_from = colrow2Square(from_col, from_row)
                    action_id = move_id[(to_col - from_col, to_row - from_row)]
                    break
    elif  len(newmove) == 5:
        from_square = square[newmove[1:3]]  
        for dir in knight_direction:
            from_col, from_row = to_col + dir[0], to_row + dir[1]
            if outOfBoard(from_col, from_row):
                continue
            if colrow2Square(from_col, from_row) != from_square:
                continue
            else:
                if board[from_square] == knight:
                    move_from = from_square
                    action_id = move_id[(to_col - from_col, to_row - from_row)]

    if move_from == -1:
        showBoard()
        print(f'actKnight: move_from = -1, move[{move}], newmove[{newmove}]')
        assert move_from != -1, f'actKnight: move_from = -1, move[{move}], newmove[{newmove}]'
    board[move_from] = '-'
    board[move_to] = knight
    ret_str = squareIDtoSTR[move_from] + squareIDtoSTR[move_to]
    return ret_str
    # print(f'{move}[{move_from * 73 + action_id}]')
    # showBoard()
    return move_from * 73 + action_id

def actBRQ(brq, move):
    newmove = removeCheckTakeMate(move)
    move_to = square[newmove[-2:]]
    move_from = -1
    to_row, to_col = square2RC(newmove[-2:])
    if brq in 'bB':
        target_direction = [direction[i] for i in range(1, 8, 2)]
    elif brq in 'rR':
        target_direction = [direction[i] for i in range(0, 8, 2)]
    else: 
        target_direction = [direction[i] for i in range(8)]
    for dir in target_direction:
        for i in range(1, 8):
            from_col, from_row = to_col + dir[0]*i, to_row + dir[1]*i
            if outOfBoard(from_col, from_row) or (board[colrow2Square(from_col, from_row)] != '-' and board[colrow2Square(from_col, from_row)] != brq):
                break
            if len(newmove) == 4:
                from_line = newmove[1]
                if from_line.isdigit():
                    if from_row != ord(from_line) - ord('1'):
                        if board[colrow2Square(from_col, from_row)] == brq:
                            break
                        continue
                elif from_line.isalpha():
                    if from_col != ord(from_line) - ord('a'):
                        if board[colrow2Square(from_col, from_row)] == brq:
                            break
                        continue
            elif len(newmove) == 5:
                from_square = square[newmove[1:3]]  
                if colrow2Square(from_col, from_row) != from_square:
                    continue
            if board[colrow2Square(from_col, from_row)] == brq:
                if pinned[colrow2Square(from_col, from_row)] == True:
                    if brq.isupper():
                        king_row, king_col = squareID2RC(king_pos[0])
                    else:
                        king_row, king_col = squareID2RC(king_pos[1])

                    dir_kf = (king_col - from_col, king_row - from_row)
                    if dir_kf[0] == 0:
                        if not dir[0] == 0:
                            break
                    elif dir_kf[1] == 0:
                        if not dir[1] == 0:
                            break
                    elif dir_kf[0] * dir_kf[1] > 0:
                        if not dir[0] * dir[1] > 0:
                            break
                    else:
                        if not dir[0] * dir[1] < 0:
                            break
        
                move_from = colrow2Square(from_col, from_row)
                action_id = move_id[(to_col - from_col, to_row - from_row)]
                break
        if move_from != -1:
            break
    if move_from == -1:
        showBoard()
        print(f'actBRQ: move_from = -1, move[{move}], newmove[{newmove}]')
        assert move_from != -1, f'actBRQ: move_from = -1, move[{move}], newmove[{newmove}]'
    board[move_from] = '-'
    board[move_to] = brq
    
    ret_str = squareIDtoSTR[move_from] + squareIDtoSTR[move_to]
    return ret_str
    # print(f'{move}[{move_from * 73 + action_id}]')
    # showBoard()
    return move_from * 73 + action_id
    
def actKing(king, move):
    newmove = removeCheckTakeMate(move)
    move_to = square[newmove[1:3]]
    move_from = -1
    to_row, to_col = square2RC(newmove[1:3])
    for dir in direction:
        from_col, from_row = to_col + dir[0], to_row + dir[1]
        if outOfBoard(from_col, from_row):
            continue
        else:
            if board[colrow2Square(from_col, from_row)] == king:
                move_from = colrow2Square(from_col, from_row)
                action_id = move_id[(to_col - from_col, to_row - from_row)]
                break
    if move_from == -1:
        showBoard()
        assert move_from != -1, f'actKing: move_from = -1, move[{move}]'
    board[move_from] = '-'
    board[move_to] = king
    if king == 'K':
        king_pos[0] = move_to
    else:
        king_pos[1] = move_to
    # print(f'{move}[{move_from * 73 + action_id}]')
    # showBoard()
        
    ret_str = squareIDtoSTR[move_from] + squareIDtoSTR[move_to]
    return ret_str
    return move_from * 73 + action_id

#dict for shallow blue's action string id pair
stringint_dict = {
    "a1a2": "0", "a1a3": "1", "a1a4": "2", "a1a5": "3", "a1a6": "4", "a1a7": "5", "a1a8": "6", "a1b2": "7", "a1c3": "8", "a1d4": "9", 
    "a1e5": "10", "a1f6": "11", "a1g7": "12", "a1h8": "13", "a1b1": "14", "a1c1": "15", "a1d1": "16", "a1e1": "17", "a1f1": "18", "a1g1": "19", 
    "a1h1": "20", "a1b3": "21", "a1c2": "22", "b1b2": "23", "b1b3": "24", "b1b4": "25", "b1b5": "26", "b1b6": "27", "b1b7": "28", "b1b8": "29", 
    "b1c2": "30", "b1d3": "31", "b1e4": "32", "b1f5": "33", "b1g6": "34", "b1h7": "35", "b1c1": "36", "b1d1": "37", "b1e1": "38", "b1f1": "39", 
    "b1g1": "40", "b1h1": "41", "b1a1": "42", "b1a2": "43", "b1c3": "44", "b1d2": "45", "b1a3": "46", "c1c2": "47", "c1c3": "48", "c1c4": "49", 
    "c1c5": "50", "c1c6": "51", "c1c7": "52", "c1c8": "53", "c1d2": "54", "c1e3": "55", "c1f4": "56", "c1g5": "57", "c1h6": "58", "c1d1": "59", 
    "c1e1": "60", "c1f1": "61", "c1g1": "62", "c1h1": "63", "c1b1": "64", "c1a1": "65", "c1b2": "66", "c1a3": "67", "c1d3": "68", "c1e2": "69", 
    "c1a2": "70", "c1b3": "71", "d1d2": "72", "d1d3": "73", "d1d4": "74", "d1d5": "75", "d1d6": "76", "d1d7": "77", "d1d8": "78", "d1e2": "79", 
    "d1f3": "80", "d1g4": "81", "d1h5": "82", "d1e1": "83", "d1f1": "84", "d1g1": "85", "d1h1": "86", "d1c1": "87", "d1b1": "88", "d1a1": "89", 
    "d1c2": "90", "d1b3": "91", "d1a4": "92", "d1e3": "93", "d1f2": "94", "d1b2": "95", "d1c3": "96", "e1e2": "97", "e1e3": "98", "e1e4": "99", 
    "e1e5": "100", "e1e6": "101", "e1e7": "102", "e1e8": "103", "e1f2": "104", "e1g3": "105", "e1h4": "106", "e1f1": "107", "e1g1": "108", "e1h1": "109", 
    "e1d1": "110", "e1c1": "111", "e1b1": "112", "e1a1": "113", "e1d2": "114", "e1c3": "115", "e1b4": "116", "e1a5": "117", "e1f3": "118", "e1g2": "119", 
    "e1c2": "120", "e1d3": "121", "f1f2": "122", "f1f3": "123", "f1f4": "124", "f1f5": "125", "f1f6": "126", "f1f7": "127", "f1f8": "128", "f1g2": "129", 
    "f1h3": "130", "f1g1": "131", "f1h1": "132", "f1e1": "133", "f1d1": "134", "f1c1": "135", "f1b1": "136", "f1a1": "137", "f1e2": "138", "f1d3": "139", 
    "f1c4": "140", "f1b5": "141", "f1a6": "142", "f1g3": "143", "f1h2": "144", "f1d2": "145", "f1e3": "146", "g1g2": "147", "g1g3": "148", "g1g4": "149", 
    "g1g5": "150", "g1g6": "151", "g1g7": "152", "g1g8": "153", "g1h2": "154", "g1h1": "155", "g1f1": "156", "g1e1": "157", "g1d1": "158", "g1c1": "159", 
    "g1b1": "160", "g1a1": "161", "g1f2": "162", "g1e3": "163", "g1d4": "164", "g1c5": "165", "g1b6": "166", "g1a7": "167", "g1h3": "168", "g1e2": "169", 
    "g1f3": "170", "h1h2": "171", "h1h3": "172", "h1h4": "173", "h1h5": "174", "h1h6": "175", "h1h7": "176", "h1h8": "177", "h1g1": "178", "h1f1": "179", 
    "h1e1": "180", "h1d1": "181", "h1c1": "182", "h1b1": "183", "h1a1": "184", "h1g2": "185", "h1f3": "186", "h1e4": "187", "h1d5": "188", "h1c6": "189", 
    "h1b7": "190", "h1a8": "191", "h1f2": "192", "h1g3": "193", "a2a3": "194", "a2a4": "195", "a2a5": "196", "a2a6": "197", "a2a7": "198", "a2a8": "199", 
    "a2b3": "200", "a2c4": "201", "a2d5": "202", "a2e6": "203", "a2f7": "204", "a2g8": "205", "a2b2": "206", "a2c2": "207", "a2d2": "208", "a2e2": "209", 
    "a2f2": "210", "a2g2": "211", "a2h2": "212", "a2b1": "213", "a2a1": "214", "a2b4": "215", "a2c3": "216", "a2c1": "217", "a2a1q": "218", "a2a1r": "219", 
    "a2a1b": "220", "a2a1n": "221", "a2b1q": "222", "a2b1r": "223", "a2b1b": "224", "a2b1n": "225", "b2b3": "226", "b2b4": "227", "b2b5": "228", "b2b6": "229", 
    "b2b7": "230", "b2b8": "231", "b2c3": "232", "b2d4": "233", "b2e5": "234", "b2f6": "235", "b2g7": "236", "b2h8": "237", "b2c2": "238", "b2d2": "239", 
    "b2e2": "240", "b2f2": "241", "b2g2": "242", "b2h2": "243", "b2c1": "244", "b2b1": "245", "b2a1": "246", "b2a2": "247", "b2a3": "248", "b2c4": "249", 
    "b2d3": "250", "b2d1": "251", "b2a4": "252", "b2a1q": "253", "b2a1r": "254", "b2a1b": "255", "b2a1n": "256", "b2b1q": "257", "b2b1r": "258", "b2b1b": "259", 
    "b2b1n": "260", "b2c1q": "261", "b2c1r": "262", "b2c1b": "263", "b2c1n": "264", "c2c3": "265", "c2c4": "266", "c2c5": "267", "c2c6": "268", "c2c7": "269", 
    "c2c8": "270", "c2d3": "271", "c2e4": "272", "c2f5": "273", "c2g6": "274", "c2h7": "275", "c2d2": "276", "c2e2": "277", "c2f2": "278", "c2g2": "279", 
    "c2h2": "280", "c2d1": "281", "c2c1": "282", "c2b1": "283", "c2b2": "284", "c2a2": "285", "c2b3": "286", "c2a4": "287", "c2d4": "288", "c2e3": "289", 
    "c2e1": "290", "c2a1": "291", "c2a3": "292", "c2b4": "293", "c2b1q": "294", "c2b1r": "295", "c2b1b": "296", "c2b1n": "297", "c2c1q": "298", "c2c1r": "299", 
    "c2c1b": "300", "c2c1n": "301", "c2d1q": "302", "c2d1r": "303", "c2d1b": "304", "c2d1n": "305", "d2d3": "306", "d2d4": "307", "d2d5": "308", "d2d6": "309", 
    "d2d7": "310", "d2d8": "311", "d2e3": "312", "d2f4": "313", "d2g5": "314", "d2h6": "315", "d2e2": "316", "d2f2": "317", "d2g2": "318", "d2h2": "319", 
    "d2e1": "320", "d2d1": "321", "d2c1": "322", "d2c2": "323", "d2b2": "324", "d2a2": "325", "d2c3": "326", "d2b4": "327", "d2a5": "328", "d2e4": "329", 
    "d2f3": "330", "d2f1": "331", "d2b1": "332", "d2b3": "333", "d2c4": "334", "d2c1q": "335", "d2c1r": "336", "d2c1b": "337", "d2c1n": "338", "d2d1q": "339", 
    "d2d1r": "340", "d2d1b": "341", "d2d1n": "342", "d2e1q": "343", "d2e1r": "344", "d2e1b": "345", "d2e1n": "346", "e2e3": "347", "e2e4": "348", "e2e5": "349", 
    "e2e6": "350", "e2e7": "351", "e2e8": "352", "e2f3": "353", "e2g4": "354", "e2h5": "355", "e2f2": "356", "e2g2": "357", "e2h2": "358", "e2f1": "359", 
    "e2e1": "360", "e2d1": "361", "e2d2": "362", "e2c2": "363", "e2b2": "364", "e2a2": "365", "e2d3": "366", "e2c4": "367", "e2b5": "368", "e2a6": "369", 
    "e2f4": "370", "e2g3": "371", "e2g1": "372", "e2c1": "373", "e2c3": "374", "e2d4": "375", "e2d1q": "376", "e2d1r": "377", "e2d1b": "378", "e2d1n": "379", 
    "e2e1q": "380", "e2e1r": "381", "e2e1b": "382", "e2e1n": "383", "e2f1q": "384", "e2f1r": "385", "e2f1b": "386", "e2f1n": "387", "f2f3": "388", "f2f4": "389", 
    "f2f5": "390", "f2f6": "391", "f2f7": "392", "f2f8": "393", "f2g3": "394", "f2h4": "395", "f2g2": "396", "f2h2": "397", "f2g1": "398", "f2f1": "399", 
    "f2e1": "400", "f2e2": "401", "f2d2": "402", "f2c2": "403", "f2b2": "404", "f2a2": "405", "f2e3": "406", "f2d4": "407", "f2c5": "408", "f2b6": "409", 
    "f2a7": "410", "f2g4": "411", "f2h3": "412", "f2h1": "413", "f2d1": "414", "f2d3": "415", "f2e4": "416", "f2e1q": "417", "f2e1r": "418", "f2e1b": "419", 
    "f2e1n": "420", "f2f1q": "421", "f2f1r": "422", "f2f1b": "423", "f2f1n": "424", "f2g1q": "425", "f2g1r": "426", "f2g1b": "427", "f2g1n": "428", "g2g3": "429", 
    "g2g4": "430", "g2g5": "431", "g2g6": "432", "g2g7": "433", "g2g8": "434", "g2h3": "435", "g2h2": "436", "g2h1": "437", "g2g1": "438", "g2f1": "439", 
    "g2f2": "440", "g2e2": "441", "g2d2": "442", "g2c2": "443", "g2b2": "444", "g2a2": "445", "g2f3": "446", "g2e4": "447", "g2d5": "448", "g2c6": "449", 
    "g2b7": "450", "g2a8": "451", "g2h4": "452", "g2e1": "453", "g2e3": "454", "g2f4": "455", "g2f1q": "456", "g2f1r": "457", "g2f1b": "458", "g2f1n": "459", 
    "g2g1q": "460", "g2g1r": "461", "g2g1b": "462", "g2g1n": "463", "g2h1q": "464", "g2h1r": "465", "g2h1b": "466", "g2h1n": "467", "h2h3": "468", "h2h4": "469", 
    "h2h5": "470", "h2h6": "471", "h2h7": "472", "h2h8": "473", "h2h1": "474", "h2g1": "475", "h2g2": "476", "h2f2": "477", "h2e2": "478", "h2d2": "479", 
    "h2c2": "480", "h2b2": "481", "h2a2": "482", "h2g3": "483", "h2f4": "484", "h2e5": "485", "h2d6": "486", "h2c7": "487", "h2b8": "488", "h2f1": "489", 
    "h2f3": "490", "h2g4": "491", "h2g1q": "492", "h2g1r": "493", "h2g1b": "494", "h2g1n": "495", "h2h1q": "496", "h2h1r": "497", "h2h1b": "498", "h2h1n": "499", 
    "a3a4": "500", "a3a5": "501", "a3a6": "502", "a3a7": "503", "a3a8": "504", "a3b4": "505", "a3c5": "506", "a3d6": "507", "a3e7": "508", "a3f8": "509", 
    "a3b3": "510", "a3c3": "511", "a3d3": "512", "a3e3": "513", "a3f3": "514", "a3g3": "515", "a3h3": "516", "a3b2": "517", "a3c1": "518", "a3a2": "519", 
    "a3a1": "520", "a3b5": "521", "a3c4": "522", "a3c2": "523", "a3b1": "524", "b3b4": "525", "b3b5": "526", "b3b6": "527", "b3b7": "528", "b3b8": "529", 
    "b3c4": "530", "b3d5": "531", "b3e6": "532", "b3f7": "533", "b3g8": "534", "b3c3": "535", "b3d3": "536", "b3e3": "537", "b3f3": "538", "b3g3": "539", 
    "b3h3": "540", "b3c2": "541", "b3d1": "542", "b3b2": "543", "b3b1": "544", "b3a2": "545", "b3a3": "546", "b3a4": "547", "b3c5": "548", "b3d4": "549", 
    "b3d2": "550", "b3c1": "551", "b3a1": "552", "b3a5": "553", "c3c4": "554", "c3c5": "555", "c3c6": "556", "c3c7": "557", "c3c8": "558", "c3d4": "559", 
    "c3e5": "560", "c3f6": "561", "c3g7": "562", "c3h8": "563", "c3d3": "564", "c3e3": "565", "c3f3": "566", "c3g3": "567", "c3h3": "568", "c3d2": "569", 
    "c3e1": "570", "c3c2": "571", "c3c1": "572", "c3b2": "573", "c3a1": "574", "c3b3": "575", "c3a3": "576", "c3b4": "577", "c3a5": "578", "c3d5": "579", 
    "c3e4": "580", "c3e2": "581", "c3d1": "582", "c3b1": "583", "c3a2": "584", "c3a4": "585", "c3b5": "586", "d3d4": "587", "d3d5": "588", "d3d6": "589", 
    "d3d7": "590", "d3d8": "591", "d3e4": "592", "d3f5": "593", "d3g6": "594", "d3h7": "595", "d3e3": "596", "d3f3": "597", "d3g3": "598", "d3h3": "599", 
    "d3e2": "600", "d3f1": "601", "d3d2": "602", "d3d1": "603", "d3c2": "604", "d3b1": "605", "d3c3": "606", "d3b3": "607", "d3a3": "608", "d3c4": "609", 
    "d3b5": "610", "d3a6": "611", "d3e5": "612", "d3f4": "613", "d3f2": "614", "d3e1": "615", "d3c1": "616", "d3b2": "617", "d3b4": "618", "d3c5": "619", 
    "e3e4": "620", "e3e5": "621", "e3e6": "622", "e3e7": "623", "e3e8": "624", "e3f4": "625", "e3g5": "626", "e3h6": "627", "e3f3": "628", "e3g3": "629", 
    "e3h3": "630", "e3f2": "631", "e3g1": "632", "e3e2": "633", "e3e1": "634", "e3d2": "635", "e3c1": "636", "e3d3": "637", "e3c3": "638", "e3b3": "639", 
    "e3a3": "640", "e3d4": "641", "e3c5": "642", "e3b6": "643", "e3a7": "644", "e3f5": "645", "e3g4": "646", "e3g2": "647", "e3f1": "648", "e3d1": "649", 
    "e3c2": "650", "e3c4": "651", "e3d5": "652", "f3f4": "653", "f3f5": "654", "f3f6": "655", "f3f7": "656", "f3f8": "657", "f3g4": "658", "f3h5": "659", 
    "f3g3": "660", "f3h3": "661", "f3g2": "662", "f3h1": "663", "f3f2": "664", "f3f1": "665", "f3e2": "666", "f3d1": "667", "f3e3": "668", "f3d3": "669", 
    "f3c3": "670", "f3b3": "671", "f3a3": "672", "f3e4": "673", "f3d5": "674", "f3c6": "675", "f3b7": "676", "f3a8": "677", "f3g5": "678", "f3h4": "679", 
    "f3h2": "680", "f3g1": "681", "f3e1": "682", "f3d2": "683", "f3d4": "684", "f3e5": "685", "g3g4": "686", "g3g5": "687", "g3g6": "688", "g3g7": "689", 
    "g3g8": "690", "g3h4": "691", "g3h3": "692", "g3h2": "693", "g3g2": "694", "g3g1": "695", "g3f2": "696", "g3e1": "697", "g3f3": "698", "g3e3": "699", 
    "g3d3": "700", "g3c3": "701", "g3b3": "702", "g3a3": "703", "g3f4": "704", "g3e5": "705", "g3d6": "706", "g3c7": "707", "g3b8": "708", "g3h5": "709", 
    "g3h1": "710", "g3f1": "711", "g3e2": "712", "g3e4": "713", "g3f5": "714", "h3h4": "715", "h3h5": "716", "h3h6": "717", "h3h7": "718", "h3h8": "719", 
    "h3h2": "720", "h3h1": "721", "h3g2": "722", "h3f1": "723", "h3g3": "724", "h3f3": "725", "h3e3": "726", "h3d3": "727", "h3c3": "728", "h3b3": "729", 
    "h3a3": "730", "h3g4": "731", "h3f5": "732", "h3e6": "733", "h3d7": "734", "h3c8": "735", "h3g1": "736", "h3f2": "737", "h3f4": "738", "h3g5": "739", 
    "a4a5": "740", "a4a6": "741", "a4a7": "742", "a4a8": "743", "a4b5": "744", "a4c6": "745", "a4d7": "746", "a4e8": "747", "a4b4": "748", "a4c4": "749", 
    "a4d4": "750", "a4e4": "751", "a4f4": "752", "a4g4": "753", "a4h4": "754", "a4b3": "755", "a4c2": "756", "a4d1": "757", "a4a3": "758", "a4a2": "759", 
    "a4a1": "760", "a4b6": "761", "a4c5": "762", "a4c3": "763", "a4b2": "764", "b4b5": "765", "b4b6": "766", "b4b7": "767", "b4b8": "768", "b4c5": "769", 
    "b4d6": "770", "b4e7": "771", "b4f8": "772", "b4c4": "773", "b4d4": "774", "b4e4": "775", "b4f4": "776", "b4g4": "777", "b4h4": "778", "b4c3": "779", 
    "b4d2": "780", "b4e1": "781", "b4b3": "782", "b4b2": "783", "b4b1": "784", "b4a3": "785", "b4a4": "786", "b4a5": "787", "b4c6": "788", "b4d5": "789", 
    "b4d3": "790", "b4c2": "791", "b4a2": "792", "b4a6": "793", "c4c5": "794", "c4c6": "795", "c4c7": "796", "c4c8": "797", "c4d5": "798", "c4e6": "799", 
    "c4f7": "800", "c4g8": "801", "c4d4": "802", "c4e4": "803", "c4f4": "804", "c4g4": "805", "c4h4": "806", "c4d3": "807", "c4e2": "808", "c4f1": "809", 
    "c4c3": "810", "c4c2": "811", "c4c1": "812", "c4b3": "813", "c4a2": "814", "c4b4": "815", "c4a4": "816", "c4b5": "817", "c4a6": "818", "c4d6": "819", 
    "c4e5": "820", "c4e3": "821", "c4d2": "822", "c4b2": "823", "c4a3": "824", "c4a5": "825", "c4b6": "826", "d4d5": "827", "d4d6": "828", "d4d7": "829", 
    "d4d8": "830", "d4e5": "831", "d4f6": "832", "d4g7": "833", "d4h8": "834", "d4e4": "835", "d4f4": "836", "d4g4": "837", "d4h4": "838", "d4e3": "839", 
    "d4f2": "840", "d4g1": "841", "d4d3": "842", "d4d2": "843", "d4d1": "844", "d4c3": "845", "d4b2": "846", "d4a1": "847", "d4c4": "848", "d4b4": "849", 
    "d4a4": "850", "d4c5": "851", "d4b6": "852", "d4a7": "853", "d4e6": "854", "d4f5": "855", "d4f3": "856", "d4e2": "857", "d4c2": "858", "d4b3": "859", 
    "d4b5": "860", "d4c6": "861", "e4e5": "862", "e4e6": "863", "e4e7": "864", "e4e8": "865", "e4f5": "866", "e4g6": "867", "e4h7": "868", "e4f4": "869", 
    "e4g4": "870", "e4h4": "871", "e4f3": "872", "e4g2": "873", "e4h1": "874", "e4e3": "875", "e4e2": "876", "e4e1": "877", "e4d3": "878", "e4c2": "879", 
    "e4b1": "880", "e4d4": "881", "e4c4": "882", "e4b4": "883", "e4a4": "884", "e4d5": "885", "e4c6": "886", "e4b7": "887", "e4a8": "888", "e4f6": "889", 
    "e4g5": "890", "e4g3": "891", "e4f2": "892", "e4d2": "893", "e4c3": "894", "e4c5": "895", "e4d6": "896", "f4f5": "897", "f4f6": "898", "f4f7": "899", 
    "f4f8": "900", "f4g5": "901", "f4h6": "902", "f4g4": "903", "f4h4": "904", "f4g3": "905", "f4h2": "906", "f4f3": "907", "f4f2": "908", "f4f1": "909", 
    "f4e3": "910", "f4d2": "911", "f4c1": "912", "f4e4": "913", "f4d4": "914", "f4c4": "915", "f4b4": "916", "f4a4": "917", "f4e5": "918", "f4d6": "919", 
    "f4c7": "920", "f4b8": "921", "f4g6": "922", "f4h5": "923", "f4h3": "924", "f4g2": "925", "f4e2": "926", "f4d3": "927", "f4d5": "928", "f4e6": "929", 
    "g4g5": "930", "g4g6": "931", "g4g7": "932", "g4g8": "933", "g4h5": "934", "g4h4": "935", "g4h3": "936", "g4g3": "937", "g4g2": "938", "g4g1": "939", 
    "g4f3": "940", "g4e2": "941", "g4d1": "942", "g4f4": "943", "g4e4": "944", "g4d4": "945", "g4c4": "946", "g4b4": "947", "g4a4": "948", "g4f5": "949", 
    "g4e6": "950", "g4d7": "951", "g4c8": "952", "g4h6": "953", "g4h2": "954", "g4f2": "955", "g4e3": "956", "g4e5": "957", "g4f6": "958", "h4h5": "959", 
    "h4h6": "960", "h4h7": "961", "h4h8": "962", "h4h3": "963", "h4h2": "964", "h4h1": "965", "h4g3": "966", "h4f2": "967", "h4e1": "968", "h4g4": "969", 
    "h4f4": "970", "h4e4": "971", "h4d4": "972", "h4c4": "973", "h4b4": "974", "h4a4": "975", "h4g5": "976", "h4f6": "977", "h4e7": "978", "h4d8": "979", 
    "h4g2": "980", "h4f3": "981", "h4f5": "982", "h4g6": "983", "a5a6": "984", "a5a7": "985", "a5a8": "986", "a5b6": "987", "a5c7": "988", "a5d8": "989", 
    "a5b5": "990", "a5c5": "991", "a5d5": "992", "a5e5": "993", "a5f5": "994", "a5g5": "995", "a5h5": "996", "a5b4": "997", "a5c3": "998", "a5d2": "999", 
    "a5e1": "1000", "a5a4": "1001", "a5a3": "1002", "a5a2": "1003", "a5a1": "1004", "a5b7": "1005", "a5c6": "1006", "a5c4": "1007", "a5b3": "1008", "b5b6": "1009", 
    "b5b7": "1010", "b5b8": "1011", "b5c6": "1012", "b5d7": "1013", "b5e8": "1014", "b5c5": "1015", "b5d5": "1016", "b5e5": "1017", "b5f5": "1018", "b5g5": "1019", 
    "b5h5": "1020", "b5c4": "1021", "b5d3": "1022", "b5e2": "1023", "b5f1": "1024", "b5b4": "1025", "b5b3": "1026", "b5b2": "1027", "b5b1": "1028", "b5a4": "1029", 
    "b5a5": "1030", "b5a6": "1031", "b5c7": "1032", "b5d6": "1033", "b5d4": "1034", "b5c3": "1035", "b5a3": "1036", "b5a7": "1037", "c5c6": "1038", "c5c7": "1039", 
    "c5c8": "1040", "c5d6": "1041", "c5e7": "1042", "c5f8": "1043", "c5d5": "1044", "c5e5": "1045", "c5f5": "1046", "c5g5": "1047", "c5h5": "1048", "c5d4": "1049", 
    "c5e3": "1050", "c5f2": "1051", "c5g1": "1052", "c5c4": "1053", "c5c3": "1054", "c5c2": "1055", "c5c1": "1056", "c5b4": "1057", "c5a3": "1058", "c5b5": "1059", 
    "c5a5": "1060", "c5b6": "1061", "c5a7": "1062", "c5d7": "1063", "c5e6": "1064", "c5e4": "1065", "c5d3": "1066", "c5b3": "1067", "c5a4": "1068", "c5a6": "1069", 
    "c5b7": "1070", "d5d6": "1071", "d5d7": "1072", "d5d8": "1073", "d5e6": "1074", "d5f7": "1075", "d5g8": "1076", "d5e5": "1077", "d5f5": "1078", "d5g5": "1079", 
    "d5h5": "1080", "d5e4": "1081", "d5f3": "1082", "d5g2": "1083", "d5h1": "1084", "d5d4": "1085", "d5d3": "1086", "d5d2": "1087", "d5d1": "1088", "d5c4": "1089", 
    "d5b3": "1090", "d5a2": "1091", "d5c5": "1092", "d5b5": "1093", "d5a5": "1094", "d5c6": "1095", "d5b7": "1096", "d5a8": "1097", "d5e7": "1098", "d5f6": "1099", 
    "d5f4": "1100", "d5e3": "1101", "d5c3": "1102", "d5b4": "1103", "d5b6": "1104", "d5c7": "1105", "e5e6": "1106", "e5e7": "1107", "e5e8": "1108", "e5f6": "1109", 
    "e5g7": "1110", "e5h8": "1111", "e5f5": "1112", "e5g5": "1113", "e5h5": "1114", "e5f4": "1115", "e5g3": "1116", "e5h2": "1117", "e5e4": "1118", "e5e3": "1119", 
    "e5e2": "1120", "e5e1": "1121", "e5d4": "1122", "e5c3": "1123", "e5b2": "1124", "e5a1": "1125", "e5d5": "1126", "e5c5": "1127", "e5b5": "1128", "e5a5": "1129", 
    "e5d6": "1130", "e5c7": "1131", "e5b8": "1132", "e5f7": "1133", "e5g6": "1134", "e5g4": "1135", "e5f3": "1136", "e5d3": "1137", "e5c4": "1138", "e5c6": "1139", 
    "e5d7": "1140", "f5f6": "1141", "f5f7": "1142", "f5f8": "1143", "f5g6": "1144", "f5h7": "1145", "f5g5": "1146", "f5h5": "1147", "f5g4": "1148", "f5h3": "1149", 
    "f5f4": "1150", "f5f3": "1151", "f5f2": "1152", "f5f1": "1153", "f5e4": "1154", "f5d3": "1155", "f5c2": "1156", "f5b1": "1157", "f5e5": "1158", "f5d5": "1159", 
    "f5c5": "1160", "f5b5": "1161", "f5a5": "1162", "f5e6": "1163", "f5d7": "1164", "f5c8": "1165", "f5g7": "1166", "f5h6": "1167", "f5h4": "1168", "f5g3": "1169", 
    "f5e3": "1170", "f5d4": "1171", "f5d6": "1172", "f5e7": "1173", "g5g6": "1174", "g5g7": "1175", "g5g8": "1176", "g5h6": "1177", "g5h5": "1178", "g5h4": "1179", 
    "g5g4": "1180", "g5g3": "1181", "g5g2": "1182", "g5g1": "1183", "g5f4": "1184", "g5e3": "1185", "g5d2": "1186", "g5c1": "1187", "g5f5": "1188", "g5e5": "1189", 
    "g5d5": "1190", "g5c5": "1191", "g5b5": "1192", "g5a5": "1193", "g5f6": "1194", "g5e7": "1195", "g5d8": "1196", "g5h7": "1197", "g5h3": "1198", "g5f3": "1199", 
    "g5e4": "1200", "g5e6": "1201", "g5f7": "1202", "h5h6": "1203", "h5h7": "1204", "h5h8": "1205", "h5h4": "1206", "h5h3": "1207", "h5h2": "1208", "h5h1": "1209", 
    "h5g4": "1210", "h5f3": "1211", "h5e2": "1212", "h5d1": "1213", "h5g5": "1214", "h5f5": "1215", "h5e5": "1216", "h5d5": "1217", "h5c5": "1218", "h5b5": "1219", 
    "h5a5": "1220", "h5g6": "1221", "h5f7": "1222", "h5e8": "1223", "h5g3": "1224", "h5f4": "1225", "h5f6": "1226", "h5g7": "1227", "a6a7": "1228", "a6a8": "1229", 
    "a6b7": "1230", "a6c8": "1231", "a6b6": "1232", "a6c6": "1233", "a6d6": "1234", "a6e6": "1235", "a6f6": "1236", "a6g6": "1237", "a6h6": "1238", "a6b5": "1239", 
    "a6c4": "1240", "a6d3": "1241", "a6e2": "1242", "a6f1": "1243", "a6a5": "1244", "a6a4": "1245", "a6a3": "1246", "a6a2": "1247", "a6a1": "1248", "a6b8": "1249", 
    "a6c7": "1250", "a6c5": "1251", "a6b4": "1252", "b6b7": "1253", "b6b8": "1254", "b6c7": "1255", "b6d8": "1256", "b6c6": "1257", "b6d6": "1258", "b6e6": "1259", 
    "b6f6": "1260", "b6g6": "1261", "b6h6": "1262", "b6c5": "1263", "b6d4": "1264", "b6e3": "1265", "b6f2": "1266", "b6g1": "1267", "b6b5": "1268", "b6b4": "1269", 
    "b6b3": "1270", "b6b2": "1271", "b6b1": "1272", "b6a5": "1273", "b6a6": "1274", "b6a7": "1275", "b6c8": "1276", "b6d7": "1277", "b6d5": "1278", "b6c4": "1279", 
    "b6a4": "1280", "b6a8": "1281", "c6c7": "1282", "c6c8": "1283", "c6d7": "1284", "c6e8": "1285", "c6d6": "1286", "c6e6": "1287", "c6f6": "1288", "c6g6": "1289", 
    "c6h6": "1290", "c6d5": "1291", "c6e4": "1292", "c6f3": "1293", "c6g2": "1294", "c6h1": "1295", "c6c5": "1296", "c6c4": "1297", "c6c3": "1298", "c6c2": "1299", 
    "c6c1": "1300", "c6b5": "1301", "c6a4": "1302", "c6b6": "1303", "c6a6": "1304", "c6b7": "1305", "c6a8": "1306", "c6d8": "1307", "c6e7": "1308", "c6e5": "1309", 
    "c6d4": "1310", "c6b4": "1311", "c6a5": "1312", "c6a7": "1313", "c6b8": "1314", "d6d7": "1315", "d6d8": "1316", "d6e7": "1317", "d6f8": "1318", "d6e6": "1319", 
    "d6f6": "1320", "d6g6": "1321", "d6h6": "1322", "d6e5": "1323", "d6f4": "1324", "d6g3": "1325", "d6h2": "1326", "d6d5": "1327", "d6d4": "1328", "d6d3": "1329", 
    "d6d2": "1330", "d6d1": "1331", "d6c5": "1332", "d6b4": "1333", "d6a3": "1334", "d6c6": "1335", "d6b6": "1336", "d6a6": "1337", "d6c7": "1338", "d6b8": "1339", 
    "d6e8": "1340", "d6f7": "1341", "d6f5": "1342", "d6e4": "1343", "d6c4": "1344", "d6b5": "1345", "d6b7": "1346", "d6c8": "1347", "e6e7": "1348", "e6e8": "1349", 
    "e6f7": "1350", "e6g8": "1351", "e6f6": "1352", "e6g6": "1353", "e6h6": "1354", "e6f5": "1355", "e6g4": "1356", "e6h3": "1357", "e6e5": "1358", "e6e4": "1359", 
    "e6e3": "1360", "e6e2": "1361", "e6e1": "1362", "e6d5": "1363", "e6c4": "1364", "e6b3": "1365", "e6a2": "1366", "e6d6": "1367", "e6c6": "1368", "e6b6": "1369", 
    "e6a6": "1370", "e6d7": "1371", "e6c8": "1372", "e6f8": "1373", "e6g7": "1374", "e6g5": "1375", "e6f4": "1376", "e6d4": "1377", "e6c5": "1378", "e6c7": "1379", 
    "e6d8": "1380", "f6f7": "1381", "f6f8": "1382", "f6g7": "1383", "f6h8": "1384", "f6g6": "1385", "f6h6": "1386", "f6g5": "1387", "f6h4": "1388", "f6f5": "1389", 
    "f6f4": "1390", "f6f3": "1391", "f6f2": "1392", "f6f1": "1393", "f6e5": "1394", "f6d4": "1395", "f6c3": "1396", "f6b2": "1397", "f6a1": "1398", "f6e6": "1399", 
    "f6d6": "1400", "f6c6": "1401", "f6b6": "1402", "f6a6": "1403", "f6e7": "1404", "f6d8": "1405", "f6g8": "1406", "f6h7": "1407", "f6h5": "1408", "f6g4": "1409", 
    "f6e4": "1410", "f6d5": "1411", "f6d7": "1412", "f6e8": "1413", "g6g7": "1414", "g6g8": "1415", "g6h7": "1416", "g6h6": "1417", "g6h5": "1418", "g6g5": "1419", 
    "g6g4": "1420", "g6g3": "1421", "g6g2": "1422", "g6g1": "1423", "g6f5": "1424", "g6e4": "1425", "g6d3": "1426", "g6c2": "1427", "g6b1": "1428", "g6f6": "1429", 
    "g6e6": "1430", "g6d6": "1431", "g6c6": "1432", "g6b6": "1433", "g6a6": "1434", "g6f7": "1435", "g6e8": "1436", "g6h8": "1437", "g6h4": "1438", "g6f4": "1439", 
    "g6e5": "1440", "g6e7": "1441", "g6f8": "1442", "h6h7": "1443", "h6h8": "1444", "h6h5": "1445", "h6h4": "1446", "h6h3": "1447", "h6h2": "1448", "h6h1": "1449", 
    "h6g5": "1450", "h6f4": "1451", "h6e3": "1452", "h6d2": "1453", "h6c1": "1454", "h6g6": "1455", "h6f6": "1456", "h6e6": "1457", "h6d6": "1458", "h6c6": "1459", 
    "h6b6": "1460", "h6a6": "1461", "h6g7": "1462", "h6f8": "1463", "h6g4": "1464", "h6f5": "1465", "h6f7": "1466", "h6g8": "1467", "a7a8": "1468", "a7b8": "1469", 
    "a7b7": "1470", "a7c7": "1471", "a7d7": "1472", "a7e7": "1473", "a7f7": "1474", "a7g7": "1475", "a7h7": "1476", "a7b6": "1477", "a7c5": "1478", "a7d4": "1479", 
    "a7e3": "1480", "a7f2": "1481", "a7g1": "1482", "a7a6": "1483", "a7a5": "1484", "a7a4": "1485", "a7a3": "1486", "a7a2": "1487", "a7a1": "1488", "a7c8": "1489", 
    "a7c6": "1490", "a7b5": "1491", "a7a8q": "1492", "a7a8r": "1493", "a7a8b": "1494", "a7a8n": "1495", "a7b8q": "1496", "a7b8r": "1497", "a7b8b": "1498", "a7b8n": "1499", 
    "b7b8": "1500", "b7c8": "1501", "b7c7": "1502", "b7d7": "1503", "b7e7": "1504", "b7f7": "1505", "b7g7": "1506", "b7h7": "1507", "b7c6": "1508", "b7d5": "1509", 
    "b7e4": "1510", "b7f3": "1511", "b7g2": "1512", "b7h1": "1513", "b7b6": "1514", "b7b5": "1515", "b7b4": "1516", "b7b3": "1517", "b7b2": "1518", "b7b1": "1519", 
    "b7a6": "1520", "b7a7": "1521", "b7a8": "1522", "b7d8": "1523", "b7d6": "1524", "b7c5": "1525", "b7a5": "1526", "b7a8q": "1527", "b7a8r": "1528", "b7a8b": "1529", 
    "b7a8n": "1530", "b7b8q": "1531", "b7b8r": "1532", "b7b8b": "1533", "b7b8n": "1534", "b7c8q": "1535", "b7c8r": "1536", "b7c8b": "1537", "b7c8n": "1538", "c7c8": "1539", 
    "c7d8": "1540", "c7d7": "1541", "c7e7": "1542", "c7f7": "1543", "c7g7": "1544", "c7h7": "1545", "c7d6": "1546", "c7e5": "1547", "c7f4": "1548", "c7g3": "1549", 
    "c7h2": "1550", "c7c6": "1551", "c7c5": "1552", "c7c4": "1553", "c7c3": "1554", "c7c2": "1555", "c7c1": "1556", "c7b6": "1557", "c7a5": "1558", "c7b7": "1559", 
    "c7a7": "1560", "c7b8": "1561", "c7e8": "1562", "c7e6": "1563", "c7d5": "1564", "c7b5": "1565", "c7a6": "1566", "c7a8": "1567", "c7b8q": "1568", "c7b8r": "1569", 
    "c7b8b": "1570", "c7b8n": "1571", "c7c8q": "1572", "c7c8r": "1573", "c7c8b": "1574", "c7c8n": "1575", "c7d8q": "1576", "c7d8r": "1577", "c7d8b": "1578", "c7d8n": "1579", 
    "d7d8": "1580", "d7e8": "1581", "d7e7": "1582", "d7f7": "1583", "d7g7": "1584", "d7h7": "1585", "d7e6": "1586", "d7f5": "1587", "d7g4": "1588", "d7h3": "1589", 
    "d7d6": "1590", "d7d5": "1591", "d7d4": "1592", "d7d3": "1593", "d7d2": "1594", "d7d1": "1595", "d7c6": "1596", "d7b5": "1597", "d7a4": "1598", "d7c7": "1599", 
    "d7b7": "1600", "d7a7": "1601", "d7c8": "1602", "d7f8": "1603", "d7f6": "1604", "d7e5": "1605", "d7c5": "1606", "d7b6": "1607", "d7b8": "1608", "d7c8q": "1609", 
    "d7c8r": "1610", "d7c8b": "1611", "d7c8n": "1612", "d7d8q": "1613", "d7d8r": "1614", "d7d8b": "1615", "d7d8n": "1616", "d7e8q": "1617", "d7e8r": "1618", "d7e8b": "1619", 
    "d7e8n": "1620", "e7e8": "1621", "e7f8": "1622", "e7f7": "1623", "e7g7": "1624", "e7h7": "1625", "e7f6": "1626", "e7g5": "1627", "e7h4": "1628", "e7e6": "1629", 
    "e7e5": "1630", "e7e4": "1631", "e7e3": "1632", "e7e2": "1633", "e7e1": "1634", "e7d6": "1635", "e7c5": "1636", "e7b4": "1637", "e7a3": "1638", "e7d7": "1639", 
    "e7c7": "1640", "e7b7": "1641", "e7a7": "1642", "e7d8": "1643", "e7g8": "1644", "e7g6": "1645", "e7f5": "1646", "e7d5": "1647", "e7c6": "1648", "e7c8": "1649", 
    "e7d8q": "1650", "e7d8r": "1651", "e7d8b": "1652", "e7d8n": "1653", "e7e8q": "1654", "e7e8r": "1655", "e7e8b": "1656", "e7e8n": "1657", "e7f8q": "1658", "e7f8r": "1659", 
    "e7f8b": "1660", "e7f8n": "1661", "f7f8": "1662", "f7g8": "1663", "f7g7": "1664", "f7h7": "1665", "f7g6": "1666", "f7h5": "1667", "f7f6": "1668", "f7f5": "1669", 
    "f7f4": "1670", "f7f3": "1671", "f7f2": "1672", "f7f1": "1673", "f7e6": "1674", "f7d5": "1675", "f7c4": "1676", "f7b3": "1677", "f7a2": "1678", "f7e7": "1679", 
    "f7d7": "1680", "f7c7": "1681", "f7b7": "1682", "f7a7": "1683", "f7e8": "1684", "f7h8": "1685", "f7h6": "1686", "f7g5": "1687", "f7e5": "1688", "f7d6": "1689", 
    "f7d8": "1690", "f7e8q": "1691", "f7e8r": "1692", "f7e8b": "1693", "f7e8n": "1694", "f7f8q": "1695", "f7f8r": "1696", "f7f8b": "1697", "f7f8n": "1698", "f7g8q": "1699", 
    "f7g8r": "1700", "f7g8b": "1701", "f7g8n": "1702", "g7g8": "1703", "g7h8": "1704", "g7h7": "1705", "g7h6": "1706", "g7g6": "1707", "g7g5": "1708", "g7g4": "1709", 
    "g7g3": "1710", "g7g2": "1711", "g7g1": "1712", "g7f6": "1713", "g7e5": "1714", "g7d4": "1715", "g7c3": "1716", "g7b2": "1717", "g7a1": "1718", "g7f7": "1719", 
    "g7e7": "1720", "g7d7": "1721", "g7c7": "1722", "g7b7": "1723", "g7a7": "1724", "g7f8": "1725", "g7h5": "1726", "g7f5": "1727", "g7e6": "1728", "g7e8": "1729", 
    "g7f8q": "1730", "g7f8r": "1731", "g7f8b": "1732", "g7f8n": "1733", "g7g8q": "1734", "g7g8r": "1735", "g7g8b": "1736", "g7g8n": "1737", "g7h8q": "1738", "g7h8r": "1739", 
    "g7h8b": "1740", "g7h8n": "1741", "h7h8": "1742", "h7h6": "1743", "h7h5": "1744", "h7h4": "1745", "h7h3": "1746", "h7h2": "1747", "h7h1": "1748", "h7g6": "1749", 
    "h7f5": "1750", "h7e4": "1751", "h7d3": "1752", "h7c2": "1753", "h7b1": "1754", "h7g7": "1755", "h7f7": "1756", "h7e7": "1757", "h7d7": "1758", "h7c7": "1759", 
    "h7b7": "1760", "h7a7": "1761", "h7g8": "1762", "h7g5": "1763", "h7f6": "1764", "h7f8": "1765", "h7g8q": "1766", "h7g8r": "1767", "h7g8b": "1768", "h7g8n": "1769", 
    "h7h8q": "1770", "h7h8r": "1771", "h7h8b": "1772", "h7h8n": "1773", "a8b8": "1774", "a8c8": "1775", "a8d8": "1776", "a8e8": "1777", "a8f8": "1778", "a8g8": "1779", 
    "a8h8": "1780", "a8b7": "1781", "a8c6": "1782", "a8d5": "1783", "a8e4": "1784", "a8f3": "1785", "a8g2": "1786", "a8h1": "1787", "a8a7": "1788", "a8a6": "1789", 
    "a8a5": "1790", "a8a4": "1791", "a8a3": "1792", "a8a2": "1793", "a8a1": "1794", "a8c7": "1795", "a8b6": "1796", "b8c8": "1797", "b8d8": "1798", "b8e8": "1799", 
    "b8f8": "1800", "b8g8": "1801", "b8h8": "1802", "b8c7": "1803", "b8d6": "1804", "b8e5": "1805", "b8f4": "1806", "b8g3": "1807", "b8h2": "1808", "b8b7": "1809", 
    "b8b6": "1810", "b8b5": "1811", "b8b4": "1812", "b8b3": "1813", "b8b2": "1814", "b8b1": "1815", "b8a7": "1816", "b8a8": "1817", "b8d7": "1818", "b8c6": "1819", 
    "b8a6": "1820", "c8d8": "1821", "c8e8": "1822", "c8f8": "1823", "c8g8": "1824", "c8h8": "1825", "c8d7": "1826", "c8e6": "1827", "c8f5": "1828", "c8g4": "1829", 
    "c8h3": "1830", "c8c7": "1831", "c8c6": "1832", "c8c5": "1833", "c8c4": "1834", "c8c3": "1835", "c8c2": "1836", "c8c1": "1837", "c8b7": "1838", "c8a6": "1839", 
    "c8b8": "1840", "c8a8": "1841", "c8e7": "1842", "c8d6": "1843", "c8b6": "1844", "c8a7": "1845", "d8e8": "1846", "d8f8": "1847", "d8g8": "1848", "d8h8": "1849", 
    "d8e7": "1850", "d8f6": "1851", "d8g5": "1852", "d8h4": "1853", "d8d7": "1854", "d8d6": "1855", "d8d5": "1856", "d8d4": "1857", "d8d3": "1858", "d8d2": "1859", 
    "d8d1": "1860", "d8c7": "1861", "d8b6": "1862", "d8a5": "1863", "d8c8": "1864", "d8b8": "1865", "d8a8": "1866", "d8f7": "1867", "d8e6": "1868", "d8c6": "1869", 
    "d8b7": "1870", "e8f8": "1871", "e8g8": "1872", "e8h8": "1873", "e8f7": "1874", "e8g6": "1875", "e8h5": "1876", "e8e7": "1877", "e8e6": "1878", "e8e5": "1879", 
    "e8e4": "1880", "e8e3": "1881", "e8e2": "1882", "e8e1": "1883", "e8d7": "1884", "e8c6": "1885", "e8b5": "1886", "e8a4": "1887", "e8d8": "1888", "e8c8": "1889", 
    "e8b8": "1890", "e8a8": "1891", "e8g7": "1892", "e8f6": "1893", "e8d6": "1894", "e8c7": "1895", "f8g8": "1896", "f8h8": "1897", "f8g7": "1898", "f8h6": "1899", 
    "f8f7": "1900", "f8f6": "1901", "f8f5": "1902", "f8f4": "1903", "f8f3": "1904", "f8f2": "1905", "f8f1": "1906", "f8e7": "1907", "f8d6": "1908", "f8c5": "1909", 
    "f8b4": "1910", "f8a3": "1911", "f8e8": "1912", "f8d8": "1913", "f8c8": "1914", "f8b8": "1915", "f8a8": "1916", "f8h7": "1917", "f8g6": "1918", "f8e6": "1919", 
    "f8d7": "1920", "g8h8": "1921", "g8h7": "1922", "g8g7": "1923", "g8g6": "1924", "g8g5": "1925", "g8g4": "1926", "g8g3": "1927", "g8g2": "1928", "g8g1": "1929", 
    "g8f7": "1930", "g8e6": "1931", "g8d5": "1932", "g8c4": "1933", "g8b3": "1934", "g8a2": "1935", "g8f8": "1936", "g8e8": "1937", "g8d8": "1938", "g8c8": "1939", 
    "g8b8": "1940", "g8a8": "1941", "g8h6": "1942", "g8f6": "1943", "g8e7": "1944", "h8h7": "1945", "h8h6": "1946", "h8h5": "1947", "h8h4": "1948", "h8h3": "1949", 
    "h8h2": "1950", "h8h1": "1951", "h8g7": "1952", "h8f6": "1953", "h8e5": "1954", "h8d4": "1955", "h8c3": "1956", "h8b2": "1957", "h8a1": "1958", "h8g8": "1959", 
    "h8f8": "1960", "h8e8": "1961", "h8d8": "1962", "h8c8": "1963", "h8b8": "1964", "h8a8": "1965", "h8g6": "1966", "h8f7": "1967", 
}
